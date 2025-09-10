-- window

function draw()
    imgui.Begin("Lua UI")
    if imgui.BeginTable("MyTable", 3) then
    -- Header row
    imgui.TableNextRow()
    imgui.TableSetColumnIndex(0)
    imgui.Text("ID")
    imgui.TableSetColumnIndex(1)
    imgui.Text("Name")
    imgui.TableSetColumnIndex(2)
    imgui.Text("Value")
    
    -- Data rows
    for i = 1, 3 do
        imgui.TableNextRow()
        imgui.TableSetColumnIndex(0)
        imgui.Text(tostring(i))
        imgui.TableNextColumn()
        imgui.Text("Item " .. i)
        imgui.TableNextColumn()
        if imgui.Button("Select##" .. i) then
            print("Selected item " .. i)
        end
        if imgui.IsItemHovered() then
            print("Hovered item " .. i)
        end
    end
    imgui.EndTable()
end
    imgui.End()
end
