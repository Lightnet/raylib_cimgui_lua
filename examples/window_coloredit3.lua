-- window 
local color3 = {1.0, 0.0, 0.0} -- RGB
print("...imgui.ColorEditFlags.NoAlpha")
print(imgui.ColorEditFlags.NoAlpha)
function draw()
    if imgui.Begin("Lua ImGui Demo", window_open) then
        imgui.Text("Hello from Lua!")
        -- ColorEdit3
        
        local new_color3, changed3 = imgui.ColorEdit3("RGB Color", color3, {imgui.ColorEditFlags.NoAlpha, imgui.ColorEditFlags.DisplayRGB})
        if changed3 then
            print("Color3 changed to:", new_color3[1], new_color3[2], new_color3[3])
            color3 = new_color3
        end

    end
    imgui.End()
end

function cleanup()
    print("Lua cleanup called")
end