#version 3.7;
#include "colors.inc"

global_settings { assumed_gamma 1.0 }

#declare Cone = true;
#declare Mesh3 = false;

#if (Cone)
light_source {
    <0,0,0>, White
    spotlight
    point_at <0,0,1>
    parallel
}

#if (Mesh3)
  #include "cone.pov"
#else
  #include "cone3.pov"
#end
object {
    Surface
    pigment { Red }
    finish { ambient 0.1 diffuse 0.5 specular 0.1 }
    rotate clock*x
    translate 2*z
}
#else
light_source {
    <0,0,0>, White
    spotlight
    point_at <0,0,1>
    parallel
}

#include "wave.pov"
object {
    Surface
    pigment { Red }
    finish { ambient 0.1 diffuse 0.5 specular 0.1 }
    rotate clock*x
    translate 25*z
}

#end
