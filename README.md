# raylib_cimgui_lua

# License: MIT

# Packages:
- raylib 5.5
- cimgui 1.92.1
- lua 5.4
- OpenGL 3.3 ( raylib)

# Information:
  This is sample project that use cimgui knowing as imgui to run with raylib but in abstraction low level by rlgl.h. Due to raylib.h can't be access to set up window and graphic.

  By using the CMake for easy compile and fetch content to get those repo download to build the application.

  Currently using the simple lua script to run imgui widgets.

```lua
function draw()
    imgui.begin_window("Lua UI")
    imgui.text("Hello from Lua!")
    imgui.end_window()
end

function cleanup()
    print("Lua cleanup called")
end
```

# Links and Credits:
- https://github.com/Lightnet/raylib_abstraction_cimgui
- https://github.com/raysan5/raylib
- https://github.com/cimgui/cimgui/blob/docking_inter/backend_test/example_glfw_opengl3/main.c
- https://github.com/WEREMSOFT/c99-raylib-cimgui-template
- https://github.com/alfredbaudisch/raylib-cimgui
- 
