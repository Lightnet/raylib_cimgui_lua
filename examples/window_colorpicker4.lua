-- window 
local ref_color = {0.5, 0.5, 0.5, 1.0}

function draw()
    if imgui.Begin("Lua ImGui Demo", window_open) then
        imgui.Text("Hello from Lua ColorEdit4!")
        -- ColorPicker4 with table flags and ref_col
        
        local new_color_picker4, changed_picker4 = imgui.ColorPicker4("RGBA Picker", color4, {
            -- imgui.ColorEditFlags.PickerHueBar,
            -- imgui.ColorEditFlags.NoTooltip
        }, ref_color)
        if changed_picker4 then
            print("Picker4 changed to:", new_color_picker4[1], new_color_picker4[2], new_color_picker4[3], new_color_picker4[4])
            ref_color = new_color_picker4
        end

    end
    imgui.End()
end

function cleanup()
    print("Lua cleanup called")
end