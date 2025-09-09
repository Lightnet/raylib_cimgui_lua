local window_open = true

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


    imgui.Begin("Lua UI", window_open, {imgui.WindowFlags.MenuBar})
        if imgui.BeginMenuBar() then
            if imgui.BeginMenu("File", true) then
                if imgui.MenuItem("Open", "Ctrl+O") then
                    -- Handle open
                    print("click")
                end
                -- imgui.set_tooltip("Open a file")
                imgui.EndMenu()
            end
            imgui.EndMenuBar()
        end
        imgui.Text("Hello from Lua!")
    imgui.End()
end

function cleanup()
    print("Lua cleanup called")
end