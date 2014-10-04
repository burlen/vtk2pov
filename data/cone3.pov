//bounds = [-0.5, 0.5, -0.5, 0.5, -0.433013, 0.433013]
#ifndef(DataRoot)
  #declare DataRoot = "./";
#end
#declare Surface = object {
mesh3 {
vertex_vectors { 8, concat(DataRoot, "cone3.povv") }
face_indices { 10, concat(DataRoot, "cone3.povt") }
}
}
