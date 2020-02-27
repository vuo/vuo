Outputs `true` if the value is not empty:

| Type          | Meaning                                                                                     |
| ------------- | ------------------------------------------------------------------------------------------- |
| Audio Samples | `true` if sample count is greater than zero                                                 |
| Cursor        | `true` if the mouse cursor is populated (takes any visible form)                            |
| Image         | `true` if width and height are greater than zero                                            |
| Layer         | `true` if the layer or any of its children can can potentially render graphics              |
| Scene Object  | `true` if the 3D object or any of its children can can potentially render graphics          |
| Shader        | `true` if the shader is anything other than the default (blue/purple gradient checkerboard) |
| Text          | `true` if the text contains at least one character (including whitespace)                   |
