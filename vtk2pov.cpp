#include <vtkPolyData.h>
#include <vtkTriangleFilter.h>
#include <vtkPointData.h>
#include <vtkCellArray.h>
#include <vtkXMLPPolyDataReader.h>
#include <vtkCellArray.h>
#include <vtkDataArray.h>

#include <fstream>
#include <string>
#include <iostream>
using std::cerr;
using std::endl;
using std::string;
using std::ofstream;

#ifdef USE_BOOST
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
#endif

// ----------------------------------------------------------------------------
template <typename T>
void streamVectors(ostream &os, size_t ntup, T *pts)
{
  for (size_t i = 0; i < ntup; ++i)
  {
    size_t ii = 3*i;
    os << "<" << pts[ii] << ", " << pts[ii+1] << ", " << pts[ii+2] << ">" << endl;
  }
}

// ----------------------------------------------------------------------------
int main(int argc, char **argv)
{
  string input;
  string output;
  string mesh = "mesh";

#ifdef USE_BOOST
  // parse command line options
  po::variables_map opts;
  po::options_description optDef("Options");
  optDef.add_options()
      ("help", "print this message")
      ("name", po::value<string>(&mesh), "name of mesh")
      ("input", po::value<string>(&input), "vtk file to process")
      ("output", po::value<string>(&output), "pov file to write to");
  try
  {
    po::store(po::parse_command_line(argc, argv, optDef), opts);
    po::notify(opts);

    if (opts.count("help"))
    {
      PrintUsage(optDef);
      return -1;
    }
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
  mesh = argc>3?argv[3]:"mesh";
#endif

  vtkXMLPPolyDataReader *r = vtkXMLPPolyDataReader::New();
  r->SetFileName(input.c_str());

  vtkTriangleFilter *tf = vtkTriangleFilter::New();
  tf->SetInputConnection(r->GetOutputPort());
  tf->Update();

  vtkPolyData *data = tf->GetOutput();
  data->Register(0);

  tf->Delete();
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
    << nPts << "," << endl;

  vtkDataArray *pts = points->GetData();
  switch (points->GetDataType())
  {
  vtkTemplateMacro(
      streamVectors(pov, nPts, static_cast<VTK_TT*>(pts->GetVoidPointer(0)));
      );
  }
  pov << "}" << endl;

  // write normals
  vtkDataArray *normals = data->GetPointData()->GetArray("Normals");

  pov << "normal_vectors {" << endl
    <<  nPts << "," << endl;

  switch (normals->GetDataType())
  {
  vtkTemplateMacro(
      streamVectors(pov, nPts, static_cast<VTK_TT*>(pts->GetVoidPointer(0)));
      );
  }
  pov << "}" << endl;

  // faces
  vtkCellArray *tris = data->GetPolys();
  size_t nTris = tris->GetNumberOfCells();

  pov << "face_indices {" << endl
    << nTris << endl;

  vtkIdType *pTris = data->GetPolys()->GetPointer();
  for (size_t i = 0; i < nTris; ++i)
  {
    size_t ii = 4*i;
    if (pTris[ii] != 3)
    {
      cerr << "cell " << i << " not a triangle!" << endl;
      continue;
    }
    pov << "<" << pTris[ii+1] << ", " << pTris[ii+2] << ", " << pTris[ii+3] << ">" << endl;
  }
  pov << "}" << endl;
  pov << "}" << endl;

  return 0;
}
