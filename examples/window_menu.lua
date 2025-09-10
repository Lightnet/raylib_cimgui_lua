-- window menu
-- main menu

function draw()
    imgui.BeginMainMenuBar()
    if imgui.BeginMenu("File", true) then
        if imgui.MenuItem("Open", "Ctrl+O") then
            -- Handle open
        end
        imgui.SetTooltip("Open a file")
        imgui.EndMenu()
    end
    imgui.EndMainMenuBar()

    imgui.Begin("Lua UI")

    imgui.Text("Hello from Lua!")
    imgui.End()
end

function cleanup()
    print("Lua cleanup called")
end