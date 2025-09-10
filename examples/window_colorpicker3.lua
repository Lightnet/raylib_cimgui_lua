-- window 
local color3 = {0.0, 1.0, 0.0} -- RGB

function draw()
    if imgui.Begin("Lua ImGui Demo", window_open) then
        imgui.Text("Hello from Lua ColorEdit4!")
        -- ColorPicker3 with table flags
        
        local new_color3, changed3 = imgui.ColorPicker3("RGB Picker", color3, {
            imgui.ColorEditFlags.PickerHueWheel,
            imgui.ColorEditFlags.NoInputs
        })
        if changed3 then
            print("Color3 changed to:", new_color3[1], new_color3[2], new_color3[3])
        end

    end
    imgui.End()
end

function cleanup()
    print("Lua cleanup called")
end