# raylib_cimgui_lua

# License: MIT

# Status:
- work in progress.
- prototype code.

# Language: c

# Information:
  This is sample project that use cimgui (c language programing) convert from imgui (c++ language programing ) to run with raylib but in abstraction low level by rlgl.h. Due to raylib.h can't be access to set up window and graphic.

  By using the CMake for easy compile and fetch content to get those repo download to build the application. Reason is that there raylib have different type build compiler.

  Currently using the simple lua script to run imgui widgets.

# Packages:
- raylib 5.5
- cimgui 1.92.1 ( imgui )
- lua 5.4
- OpenGL 3.3 ( raylib and local OS if driver pacakge exist )
- glfw ( raylib )
- CMake

# Tools:
- msys64 (toolchain compiler)
- CMake

  Current config for windows with msys64 for small compiler. For later use for cross platform OS for desktop tests.

# Important Notes:
```
imgui.end()
```
  This does not work due to match end for if statement conflict due how lua api table work. To follow imgui format for easy to read and port to c/c++ to be use later.

  Some change to keep it simple since c does not work on lua script. It will handle in c lua state side code.

```lua
-- set up local or theme
imgui.StyleColorsDark()

function draw()-- this get call from c main loop.
    imgui.Begin("Lua UI")
    imgui.Text("Hello from Lua!")
    imgui.End()
end

function cleanup()-- not work on yet
    print("Lua cleanup called")
end
```

# Note:
- This is place holder code still need to make sure it coded and tested.

# Features:
- [ ] flags
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
- [x] ColorEdit3
- [x] ColorEdit4
- [x] ColorPicker3
- [x] ColorPicker4
- [x] ColorButton
- [x] BeginTabBar
- [x] EndTabBar
- [x] BeginTabItem
- [x] EndTabItem
- [x] IsItemHovered
- [x] IsItemActive
- [x] IsItemClicked
- [x] SameLine
- [x] Separator
- [x] Spacing
- [x] BeginTable
- [x] EndTable
- [x] TableNextRow
- [x] TableNextColumn
- [x] TableSetColumnIndex
- [x] InputText
- [x] InputTextMultiline
- [x] RadioButton
- [x] ProgressBar
- [x] TreeNode
- [x] TreePop
- [x] Bullet
- [x] TextColored
- [x] Combo
- [x] ListBox
- [x] PlotLines
- [x] PlotHistogram
- [x] VSliderFloat
- [x] BeginChild
- [x] EndChild
- [x] IsWindowAppearing
- [x] IsWindowCollapsed
- [x] IsWindowFocused
- [x] IsWindowHovered
- [x] GetWindowPos
- [x] GetWindowSize
- [x] GetWindowWidth
- [x] GetWindowHeight
- [x] CollapsingHeader

## Theme:
- [x] imgui.GetStyleCustom(imgui.Col.Text)
- [x] imgui.SetStyleCustom(imgui.Col.Text, {0.0, 0.0, 1.0, 1.0}) -- Set blue text
- [ ] imgui.SetStyleCustoms({ }) -- Table-based, not working

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

- imgui.ColorEditFlags.NoAlpha
- imgui.ColorEditFlags.'Name'

- imgui.Col.Text
- imgui.Col.WindowBg
- imgui.Col.Button
- imgui.Col.ButtonHovered
- imgui.Col.'Name'

  Work in progres.

# render 3d:
  work in progress.

# render 2d:
  work in progress.


# Network:
 Using the Enet https://github.com/zpl-c/enet without secure for learning.

 This work in progress. As been rework and need relearn how code works.

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
- Grok AI Agent on https://x.com/i/grok
