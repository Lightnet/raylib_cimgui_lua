-- window theme

function draw()
    imgui.Begin("Lua UI")
    imgui.Text("Hello from Lua!")
    if imgui.Button("dark") then
        imgui.StyleColorsDark()
    end
    if imgui.Button("light") then
        imgui.StyleColorsLight()
    end
    if imgui.Button("classic") then
        imgui.StyleColorsClassic()
    end

    if imgui.Button("imgui version") then
        print(imgui.GetVersion())
    end
    imgui.End()
end

function cleanup()
    print("Lua cleanup called")
end