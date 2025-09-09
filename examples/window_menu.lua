function draw()
    imgui.begin_main_menu_bar()
    if imgui.begin_menu("File", true) then
        if imgui.menu_item("Open", "Ctrl+O") then
            -- Handle open
        end
        imgui.set_tooltip("Open a file")
        imgui.end_menu()
    end
    imgui.end_main_menu_bar()

    imgui.begin("Lua UI")

    imgui.text("Hello from Lua!")
    imgui.End()
end

function cleanup()
    print("Lua cleanup called")
end