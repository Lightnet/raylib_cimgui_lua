# raylib_cimgui_lua

# License: MIT

# Status:
- work in progress.
- prototype code.

# Packages:
- raylib 5.5
- cimgui 1.92.1 ( imgui )
- lua 5.4
- OpenGL 3.3 ( raylib and local OS if driver pacakge exist )
- glfw ( raylib )
- CMake

# tools:
- msys64
- CMake

  Current config for windows with msys64 for small compiler. For later use for cross platform OS for desktop tests.

# Important Notes:
```
imgui.end()
```
  This does not work due to match end for if statement conflict due how table works. To follow imgui format for easy to read and port to c/c++ to be use later.

# Information:
  This is sample project that use cimgui (c language programing) knowing as imgui (c++ language programing ) to run with raylib but in abstraction low level by rlgl.h. Due to raylib.h can't be access to set up window and graphic.

  Using the cimgui since there devs convert from imgui for better c language programing.

  By using the CMake for easy compile and fetch content to get those repo download to build the application. Reason is that there raylib have different type build compiler.

  Currently using the simple lua script to run imgui widgets.

```lua
function draw()--testing
    imgui.Begin("Lua UI") -- need to change the name to cimgui for easy read.
    imgui.Text("Hello from Lua!")
    imgui.End()
end

function cleanup()--not work on yet
    print("Lua cleanup called")
end
```

# Note:
- this is place holder code still need to make sure it coded and tested.


# Features:
- [ ] 
- [x] Begin
- [x] End
- [x] Text
- [x] Textf
- [x] SliderFloat
- [x] Button
- [x] CheckBox
- [x] StyleColorsDark
- [x] StyleColorsLight
- [x] StyleColorsClassic
- [x] GetVersion
- [x] BeginMenuBar
- [x] EndMenuBar
- [x] BeginMainMenuBar
- [x] EndMainMenuBar
- [x] BeginMenu
- [x] EndMenu
- [x] MenuItem
- [x] BeginTooltip
- [x] EndTooltip
- [x] SetTooltip
- [ ] 
- [ ] 
- [ ] 
- [ ] 

# Flags:
- imgui.WindowFlags.NoTitleBar
- imgui.WindowFlags.NoResize
- imgui.WindowFlags.NoMove
- imgui.WindowFlags.NoScrollbar
- imgui.WindowFlags.NoCollapse
- imgui.WindowFlags.AlwaysAutoResize
- imgui.WindowFlags.NoBackground
- imgui.WindowFlags.NoSavedSettings
- imgui.WindowFlags.NoMouseInputs
- imgui.WindowFlags.MenuBar
- imgui.WindowFlags.HorizontalScrollbar
- imgui.WindowFlags.NoFocusOnAppearing
- imgui.WindowFlags.NoBringToFrontOnFocus
- imgui.WindowFlags.AlwaysVerticalScrollbar
- imgui.WindowFlags.AlwaysHorizontalScrollbar
- imgui.WindowFlags.NoNavInputs
- imgui.WindowFlags.NoNavFocus
- imgui.WindowFlags.NoDecoration
- imgui.WindowFlags.NoInputs
- imgui.WindowFlags.NoNav


# Dev:

main.c
```
setup window
setup graphic
setup cimgui
setup lua

loop
  input
  graphic start
  imgui start frame
  imgui render ui stuff
  imgui end frame

  camera
  render mesh

  rlDrawRenderBatchActive();
  glUseProgram(0);
  imgui render draw data
  glfwSwapBuffers(window); 
clean up
```
  There are four stage for imgui to handle imgui start, render ui, imgui end and imgui draw graphic. Well there is input to count as well. It all process under the hood.

  Lua is added after imgui reason that script need to setup theme style else it would crashed.


# Links and Credits:
- https://github.com/Lightnet/raylib_abstraction_cimgui
- https://github.com/raysan5/raylib
- https://github.com/cimgui/cimgui/blob/docking_inter/backend_test/example_glfw_opengl3/main.c
- https://github.com/WEREMSOFT/c99-raylib-cimgui-template
- https://github.com/alfredbaudisch/raylib-cimgui
- Grok AI Agent on x.com
