-- window 
local color4 = {1.0, 0.0, 0.0, 1.0} -- RGBA

function draw()
    if imgui.Begin("Lua ImGui Demo", window_open) then
        imgui.Text("Hello from Lua ColorEdit4!")
        -- ColorEdit4 with table flags
        
        local new_color4, changed4 = imgui.ColorEdit4("RGBA Color", color4, {
            imgui.ColorEditFlags.NoAlpha,
            imgui.ColorEditFlags.DisplayRGB
        })
        if changed4 then
            print("Color4 changed to:", new_color4[1], new_color4[2], new_color4[3], new_color4[4])
        end

    end
    imgui.End()
end

function cleanup()
    print("Lua cleanup called")
end