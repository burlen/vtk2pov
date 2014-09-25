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

#ifdef ENABLE_BOOST
  // parse command line options
  po::variables_map opts;
  po::options_description optDef("Options");
  optDef.add_options()
      ("help", "print this message")
      ("verbosity", po::value<int>(&verbosity), "diagnostic verbosity level (0)")
      ("name", po::value<string>(&mesh), "name of mesh (Surface)")
      ("input", po::value<string>(&input)->required(), "REQ. vtk file to process")
      ("output", po::value<string>(&output)->required(), "REQ. pov file to write to")
      ("subdivision", po::value<string>(&subdivision), "subdivision type: butterfly, loop, linear (none)")
      ("ndivisions", po::value<int>(&ndivisions), "number of subdivisions (2)")
      ("gen-normals", "generate normals (off)")
      ("pass-normals", "pass normals (off)");
  try
  {
    po::store(po::parse_command_line(argc, argv, optDef), opts);

    if (opts.count("help"))
    {
      PrintUsage(optDef);
      return -1;
    }

    po::notify(opts);

    if (opts.count("gen-normals")) genNormals = 1;
    if (opts.count("pass-normals")) passNormals = 1;
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

  if (verbosity)
  {
    cerr << "processing VTK polydata...";
  }

  vtkXMLPPolyDataReader *r = vtkXMLPPolyDataReader::New();
  r->SetFileName(input.c_str());

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

  if (verbosity)
  {
     cerr << "done" << endl
       << "writing POVRay verts...";
  }

  vtkPolyData *data = pdn->GetOutput();
  data->Register(0);

  pdn->Delete();
  tf->Delete();
  sf->Delete();
  cpd->Delete();
  r->Delete();

  ofstream pov(output.c_str());
  if (!pov.good())
  {
    cerr << "Failed to open output \"" << output <<"\"" << endl;
    return -1;
  }

  pov
    << "#declare " << mesh << " = object {" << endl
    << "mesh2 {" << endl;

  // write points
  vtkPoints *points = data->GetPoints();
  size_t nPts = points->GetNumberOfPoints();

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

  if (genNormals || passNormals)
  {
    if (verbosity)
    {
      cerr << "done" << endl
        << "writing POVRay normals...";
    }

    // write normals
    vtkDataArray *normals = data->GetPointData()->GetArray("Normals");
    if (!normals)
    {
      cerr << "Error: Normals not found" << endl;
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
    cerr << "writing POVRay facets...";
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
    cerr << "done" << endl
      << "processing complete!" << endl;
  }

  data->Delete();

  return 0;
}
