// Copyright 2012 Ashima Arts <http://ashimaarts.com/>
// License: BSD-3-Clause <http://www.opensource.org/licenses/BSD-3-Clause>

float determinant(mat2 m) {
  return m[0][0]*m[1][1] - m[1][0]*m[0][1] ;
  }

float determinant(mat3 m) {
  return   m[0][0]*( m[1][1]*m[2][2] - m[2][1]*m[1][2])
         - m[1][0]*( m[0][1]*m[2][2] - m[2][1]*m[0][2])
         + m[2][0]*( m[0][1]*m[1][2] - m[1][1]*m[0][2]) ;
  }

// 4x4 determinate inplemented by blocks ..
//     | A B |
// det | C D | = det (A) * det(D - CA'B)
//

float determinant(mat4 m) {
  mat2 a = mat2(m);
  mat2 b = mat2(m[2].xy,m[3].xy);
  mat2 c = mat2(m[0].zw,m[1].zw);
  mat2 d = mat2(m[2].zw,m[3].zw);
  float s = determinant(a);
  return s*determinant(d-(1.0/s)*c*mat2(a[1][1],-a[0][1],-a[1][0],a[0][0])*b);
  }

mat2 inverse(mat2 m) {
  float d = 1.0 / determinant(m) ;
  return d * mat2( m[1][1], -m[0][1], -m[1][0], m[0][0]) ;
  }

mat3 inverse(mat3 m) {
  float d = 1.0 / determinant(m) ;
  return d * mat3( m[2][2]*m[1][1] - m[1][2]*m[2][1],
                    m[1][2]*m[2][0] - m[2][2]*m[1][0],
                     m[2][1]*m[1][0] - m[1][1]*m[2][0] ,

                   m[0][2]*m[2][1] - m[2][2]*m[0][1],
                    m[2][2]*m[0][0] - m[0][2]*m[2][0],
                     m[0][1]*m[2][0] - m[2][1]*m[0][0],

                   m[1][2]*m[0][1] - m[0][2]*m[1][1],
                    m[0][2]*m[1][0] - m[1][2]*m[0][0],
                     m[1][1]*m[0][0] - m[0][1]*m[1][0]
                 );
  }

// This implements block wise inverse
// | e f |   | A B |'    | A'+A'B(D-CA'B)'CA'    -A'B(D-CA'B)'  |
// | g h | = | C D |   = |  -(D-CA'B)'CA'         (D-CA'B)'     |
//
// with
// a inverted immediately
// and sub expressions t = CA'
// noting that h and f are subexpression also
//
mat4 inverse(mat4 m) {
  mat2 a = inverse(mat2(m));
  mat2 b = mat2(m[2].xy,m[3].xy);
  mat2 c = mat2(m[0].zw,m[1].zw);
  mat2 d = mat2(m[2].zw,m[3].zw);

  mat2 t = c*a;
  mat2 h = inverse(d - t*b);
  mat2 g = - h*t;
  mat2 f = - a*b*h;
  mat2 e = a - f*t;

  return mat4( vec4(e[0],g[0]), vec4(e[1],g[1]),
                  vec4(f[0],h[0]), vec4(f[1],f[1]) );
  }
