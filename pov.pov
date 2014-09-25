#include "colors.inc"

light_source {
    <0,2,2>, White
    spotlight
    point_at <0,0,2>
    parallel
}

#include "cone.pov"
object {
    Surface
    pigment { Red }
    finish { ambient 0.1 diffuse 0.5 specular 0.1 }
    rotate clock*x
    translate 2*z
}
