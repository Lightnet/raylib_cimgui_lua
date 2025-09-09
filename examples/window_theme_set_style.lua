function draw()
    imgui.begin_window("Lua UI")
    imgui.text("Hello from Lua!")
    if imgui.button("dark") then
        imgui.style_colors_dark()
    end
    if imgui.button("light") then
        imgui.style_colors_light()
    end
    if imgui.button("classic") then
        imgui.style_colors_classic()
    end

    if imgui.button("imgui version") then
        print(imgui.get_version())
    end
    imgui.end_window()
end

function cleanup()
    print("Lua cleanup called")
end