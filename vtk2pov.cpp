#include <vtkPolyData.h>
#include <vtkTriangleFilter.h>
#include <vtkPointData.h>
#include <vtkCellArray.h>
#include <vtkXMLPPolyDataReader.h>
#include <vtkCellArray.h>
#include <vtkDataArray.h>
#include <vtkPolyDataNormals.h>
#include <vtkCleanPolyData.h>
#include <vtkButterflySubdivisionFilter.h>
#include <vtkLoopSubdivisionFilter.h>
#include <vtkLinearSubdivisionFilter.h>
#include <vtkPolyDataAlgorithm.h>

#include <unistd.h>
#include <errno.h>

#include <cstdio>
#include <cstring>

#include <fstream>
#include <string>
#include <iostream>
using std::cerr;
using std::endl;
using std::string;
using std::ofstream;

#ifdef ENABLE_BOOST
#include <boost/program_options.hpp>
namespace po = boost::program_options;

// ----------------------------------------------------------------------------
void PrintUsage(po::options_description &optDef)
{
  cerr
    << "usage:" << endl
    << "vtk2pov [options] input output" << endl
    << endl
    << optDef << endl
    << endl;
}
#else
#include <cstdlib>
#endif

// ----------------------------------------------------------------------------
template <typename T>
void streamVectors(FILE *pov, size_t ntup, T *pts)
{
  size_t nelem = ntup*3;
  size_t vecBufSize = 3*sizeof(float)*ntup;

  float *vecBuf = (float *)malloc(vecBufSize);

  for (size_t i = 0; i < nelem; ++i)
  {
    vecBuf[i] = pts[i];
  }

  fwrite(vecBuf, 1, vecBufSize, pov);
  free(vecBuf);
}

// ----------------------------------------------------------------------------
template <typename T>
void streamVectors(ostream &os, size_t ntup, T *pts)
{
  for (size_t i = 0; i < ntup; ++i)
  {
    size_t ii = 3*i;
    os << "," << endl
      << "<" << pts[ii] << ", " << pts[ii+1] << ", " << pts[ii+2] << ">";
  }
}

// ----------------------------------------------------------------------------
int writeMesh2(vtkPolyData *data, const string &output, const string &mesh, int verbosity, int passNormals, int genNormals)
{
  ofstream pov(output.c_str());
  if (!pov.good())
  {
    cerr << "Failed to open output \"" << output <<"\"" << endl;
    return -1;
  }

  double bounds[6];
  data->GetBounds(bounds);

  pov
    << "//bounds = [" << bounds[0] << ", " << bounds[1]
    << ", " << bounds[2] << ", " << bounds[3]
    << ", " <<  bounds[4] << ", " << bounds[5] << "]" << endl
    << "#declare " << mesh << " = object {" << endl
    << "mesh2 {" << endl;

  // write points
  vtkPoints *points = data->GetPoints();
  if (!points)
  {
    cerr << "Error: dataset had no points!" << endl;
    pov.close();
    unlink(output.c_str());
    return -1;
  }
  size_t nPts = points->GetNumberOfPoints();

  if (verbosity)
  {
    cerr << "writing verts...";
  }

  pov << "vertex_vectors {" << endl
    << nPts;

  vtkDataArray *pts = points->GetData();
  switch (points->GetDataType())
  {
  vtkTemplateMacro(
      streamVectors(pov, nPts, static_cast<VTK_TT*>(pts->GetVoidPointer(0)));
      );
  }
  pov << "}" << endl;

  if (verbosity)
  {
    cerr << "done" << endl;
  }

  if (genNormals || passNormals)
  {
    if (verbosity)
    {
      cerr << "writing normals...";
    }

    // write normals
    vtkDataArray *normals = data->GetPointData()->GetArray("Normals");
    if (!normals)
    {
      cerr << "Error: Normals not found" << endl;
      pov.close();
      unlink(output.c_str());
      return -1;
    }

    pov << "normal_vectors {" << endl
      <<  nPts;

    switch (normals->GetDataType())
    {
    vtkTemplateMacro(
        streamVectors(pov, nPts, static_cast<VTK_TT*>(pts->GetVoidPointer(0)));
        );
    }
    pov << "}" << endl;

    if (verbosity)
    {
      cerr << "done" << endl;
    }
  }

  // faces
  if (verbosity)
  {
    cerr << "writing facets...";
  }

  vtkCellArray *tris = data->GetPolys();
  size_t nTris = tris->GetNumberOfCells();

  pov << "face_indices {" << endl
    << nTris;

  vtkIdType *pTris = data->GetPolys()->GetPointer();
  for (size_t i = 0; i < nTris; ++i)
  {
    size_t ii = 4*i;
    if (pTris[ii] != 3)
    {
      cerr << "cell " << i << " not a triangle!" << endl;
      continue;
    }
    pov
      << "," << endl
      << "<" << pTris[ii+1] << ", " << pTris[ii+2] << ", " << pTris[ii+3] << ">";
  }
  pov << "}" << endl
   << "}" << endl
   << "}" << endl;

  if (verbosity)
  {
    cerr << "done" << endl;
  }

  return 0;
}

// ----------------------------------------------------------------------------
int writeMesh3(vtkPolyData *data, string &output, const string &mesh, int verbosity, int passNormals, int genNormals)
{

  if (verbosity)
  {
    cerr << "writing header...";
  }

  // for the metadata file use the full path so
  // it ends up where user intended.
  string povFileName = output;

  // strip the extension
  size_t dotPos = output.rfind('.');
  if (dotPos != string::npos)
  {
     output = output.substr(0, dotPos);
  }
  // paths where binary datafiles are written
  string vertFileName = output + ".povv";
  string triFileName = output + ".povt";
  string normFileName = output + ".povn";

  // for portability we strip off the path and use
  // a relative path from externally provided script
  // variable
  size_t slashPos = output.rfind('/');
  if (slashPos == string::npos)
  {
    slashPos = 0;
  }
  else
  {
    slashPos += 1;
  }

  FILE *povFile = fopen(povFileName.c_str(), "wb");
  if (!povFile)
  {
    const char *estr = strerror(errno);
    cerr << endl
      << "Error: Failed to open povFile \"" << povFileName <<"\"" << endl
      << estr << endl;
    return -1;
  }

  double bounds[6];
  data->GetBounds(bounds);

  // the first line of the file will have the bounding box
  // to facilitaate scene placement
  fprintf(povFile, "//bounds = [%g, %g, %g, %g, %g, %g]\n",
    bounds[0], bounds[1], bounds[2], bounds[3], bounds[4], bounds[5]);

  // for portability all the internal paths need to  be
  // relative to a user provided script var. if the user
  // doesn't set it fallback to the current dir.
  fprintf(povFile, "#ifndef(DataRoot)\n");
  fprintf(povFile, "  #declare DataRoot = \"./\";\n");
  fprintf(povFile, "#end\n");

  // we'll store the mesh in a named object to facilitate
  // further manipulations
  fprintf(povFile,"#declare %s = object {\n", mesh.c_str());
  fprintf(povFile,"mesh3 {\n");

  // the vertex_vector keyword needs the number of vertices
  // and a path to the binary datafile given relative to
  // user provided data root.
  vtkPoints *points = data->GetPoints();
  if (!points)
  {
    cerr << "Error: dataset had no points!" << endl;
    fclose(povFile);
    unlink(output.c_str());
    return -1;
  }
  size_t nPts = points->GetNumberOfPoints();

  fprintf(povFile, "vertex_vectors { %d, concat(DataRoot, \"%s\") }\n",
    nPts, vertFileName.substr(slashPos).c_str());

  // the normal_vectors keyword needs the number of normals
  // and a relative path (from user provided data root) to
  // the binary file holding the normals.
  if (genNormals || passNormals)
  {
    fprintf(povFile, "normal_vectors { %d,  concat(DataRoot, \"%s\") }\n",
      nPts, normFileName.substr(slashPos).c_str());
  }

  vtkCellArray *tris = data->GetPolys();
  size_t nTris = tris->GetNumberOfCells();

  // the face_indices keyword needs the number of triangles
  // and the relative path (from the user provided data root)
  // to the binary point list describing the faces.
  fprintf(povFile, "face_indices { %d, concat(DataRoot, \"%s\") }\n",
    nTris, triFileName.substr(slashPos).c_str());

  fprintf(povFile, "}\n");
  fprintf(povFile, "}\n");

  fclose(povFile);

  // write points in binary format
  if (verbosity)
  {
    cerr << "done" << endl
      << "writing verts...";
  }

  FILE *vertFile = fopen(vertFileName.c_str(), "wb");
  if (!vertFile)
  {
    const char *estr = strerror(errno);
    cerr << endl
      << "Error: Failed to open pov verts \"" << vertFileName <<"\"" << endl
      << estr << endl;
    unlink(povFileName.c_str());
    return -1;
  }

  vtkDataArray *pts = points->GetData();
  switch (points->GetDataType())
  {
  vtkTemplateMacro(
      streamVectors(vertFile, nPts, static_cast<VTK_TT*>(pts->GetVoidPointer(0)));
      );
  }

  fclose(vertFile);

  if (verbosity)
  {
    cerr << "done" << endl;
  }

  // write normals in binary format
  if (genNormals || passNormals)
  {
    if (verbosity)
    {
      cerr << "writing normals...";
    }

    vtkDataArray *normals = data->GetPointData()->GetArray("Normals");
    if (!normals)
    {
      cerr << endl << "Error: Normals not found" << endl;
      unlink(povFileName.c_str());
      unlink(vertFileName.c_str());
      return -1;
    }

    FILE *normFile = fopen(normFileName.c_str(), "wb");
    if (!normFile)
    {
      const char *estr = strerror(errno);
      cerr << endl
        << "Error: Failed to open pov norms \"" << normFileName <<"\"" << endl
        << estr << endl;
      unlink(povFileName.c_str());
      unlink(vertFileName.c_str());
      return -1;
    }

    switch (normals->GetDataType())
    {
    vtkTemplateMacro(
        streamVectors(normFile, nPts, static_cast<VTK_TT*>(pts->GetVoidPointer(0)));
        );
    }

    fclose(normFile);

    if (verbosity)
    {
      cerr << "done" << endl;
    }
  }

  // write the faces in a binary format
  if (verbosity)
  {
    cerr << "writing facets...";
  }

  size_t triBufSize = nTris*3*sizeof(int);
  int *triBuf = (int *)malloc(triBufSize);

  vtkIdType *pTris = data->GetPolys()->GetPointer();
  for (size_t i = 0; i < nTris; ++i)
  {
    size_t ii = 4*i;
    if (pTris[ii] != 3)
    {
      cerr << "cell " << i << " not a triangle!" << endl;
      continue;
    }

    size_t jj = i*3;
    triBuf[jj  ] = pTris[ii+1];
    triBuf[jj+1] = pTris[ii+2];
    triBuf[jj+2] = pTris[ii+3];
  }

  FILE *triFile = fopen(triFileName.c_str(), "wb");
  if (!triFile)
  {
    const char *estr = strerror(errno);
    cerr << endl
      << "Error: Failed to open pov tris \"" << triFileName <<"\"" << endl
      << estr << endl;
    unlink(povFileName.c_str());
    unlink(vertFileName.c_str());
    unlink(triFileName.c_str());
    return -1;
  }

  fwrite(triBuf, 1, triBufSize, triFile);
  free(triBuf);
  fclose(triFile);

  if (verbosity)
  {
    cerr << "done" << endl;
  }

  return 0;
}
// ----------------------------------------------------------------------------
int main(int argc, char **argv)
{
  string input;
  string output;
  string mesh = "Surface";
  string subdivision = "none";
  int ndivisions = 2;
  int verbosity = 0;
  int genNormals = 0;
  int passNormals = 0;
  int mesh3 = 0;

#ifdef ENABLE_BOOST
  // parse command line options
  po::variables_map opts;
  po::options_description optDef("Options");
  optDef.add_options()
      ("input", po::value<string>(&input)->required(), "REQ. vtk file to process")
      ("output", po::value<string>(&output)->required(), "REQ. pov file to write to")
      ("verbose", "diagnostic verbosity level (off)")
      ("name", po::value<string>(&mesh), "name of mesh (Surface)")
      ("subdivision", po::value<string>(&subdivision), "subdivision type: butterfly, loop, linear (none)")
      ("ndivisions", po::value<int>(&ndivisions), "number of subdivisions (2)")
      ("gen-normals", "generate normals (off)")
      ("pass-normals", "pass normals (off)")
      ("mesh3", "write mesh3 format (off)")
      ("help", "print this message");
  try
  {
    po::store(po::parse_command_line(argc, argv, optDef), opts);

    if (opts.count("help"))
    {
      PrintUsage(optDef);
      return -1;
    }

    po::notify(opts);

    if (opts.count("verbose")) verbosity = 1;
    if (opts.count("gen-normals")) genNormals = 1;
    if (opts.count("pass-normals")) passNormals = 1;
    if (opts.count("mesh3")) mesh3 = 1;
  }
  catch (std::exception &e)
  {
    cerr << "Error" << endl << endl
      << e.what() << endl << endl;
    PrintUsage(optDef);
    return -1;
  }
#else
  if (argc < 3)
  {
    cerr << "error: usage vtk2pov in out [name]" << endl;
    return -1;
  }
  input = argv[1];
  output = argv[2];
  mesh = argc>3?argv[3]:"Surface";
  verbosity = argc>4?atoi(argv[4]):0;
#endif


  // here's a small vtk pipeline that reads the data,
  // cleans duplicate points, and optionally, subdivides,
  // triangulates, computes or passes normals.
  if (verbosity)
  {
    cerr << "processing polydata...";
  }

  vtkXMLPPolyDataReader *r = vtkXMLPPolyDataReader::New();
  r->SetFileName(input.c_str());
  int nArrays = r->GetNumberOfCellArrays();
  for (int i = 0; i < nArrays; ++i)
  {
    r->SetCellArrayStatus(r->GetCellArrayName(i), 0);
  }
  nArrays = r->GetNumberOfPointArrays();
  for (int i = 0; i < nArrays; ++i)
  {
    r->SetPointArrayStatus(r->GetPointArrayName(i), 0);
  }
  if (passNormals)
  {
    r->SetPointArrayStatus("Normals", 1);
  }

  vtkCleanPolyData *cpd = vtkCleanPolyData::New();
  cpd->SetInputConnection(r->GetOutputPort());

  vtkPolyDataAlgorithm *sf = NULL;
  if (subdivision == "butterfly")
  {
    vtkButterflySubdivisionFilter *f = vtkButterflySubdivisionFilter::New();
    f->SetInputConnection(cpd->GetOutputPort());
    f->SetNumberOfSubdivisions(ndivisions);
    sf = f;
  }
  else if (subdivision == "loop")
  {
    vtkLoopSubdivisionFilter *f = vtkLoopSubdivisionFilter::New();
    f->SetInputConnection(cpd->GetOutputPort());
    f->SetNumberOfSubdivisions(ndivisions);
    sf = f;
  }
  else if (subdivision == "linear")
  {
    vtkLinearSubdivisionFilter *f = vtkLinearSubdivisionFilter::New();
    f->SetInputConnection(cpd->GetOutputPort());
    f->SetNumberOfSubdivisions(ndivisions);
    sf = f;
  }
  else
  {
    sf = cpd;
    cpd->Register(0); // take ref for sf->Delete
  }

  vtkTriangleFilter *tf = vtkTriangleFilter::New();
  tf->SetInputConnection(sf->GetOutputPort());

  vtkPolyDataAlgorithm *pdn = NULL;
  if (genNormals)
  {
    vtkPolyDataNormals *f = vtkPolyDataNormals::New();
    f->SetInputConnection(tf->GetOutputPort());
    f->AutoOrientNormalsOn();
    f->ConsistencyOn();
    pdn = f;
  }
  else
  {
    pdn = tf;
    tf->Register(0); // take ref for pdn->Delete
  }

  pdn->Update();

  vtkPolyData *data = pdn->GetOutput();
  data->Register(0);

  pdn->Delete();
  tf->Delete();
  sf->Delete();
  cpd->Delete();
  r->Delete();

  if (verbosity)
  {
     cerr << "done" << endl
       << "writing mesh" << (mesh3?"3":"2") << "..." << endl;
  }

  if (mesh3)
  {
    if (writeMesh3(data, output, mesh, verbosity, passNormals, genNormals))
    {
      cerr << "Failed to write data for \"" << input << "\"" << endl;
    }
  }
  else
  {
    if (writeMesh2(data, output, mesh, verbosity, passNormals, genNormals))
    {
      cerr << "Failed to write data for \"" << input << "\"" << endl;
    }
  }

  if (verbosity)
  {
    cerr << "processing complete!" << endl;
  }

  data->Delete();

  return 0;
}
