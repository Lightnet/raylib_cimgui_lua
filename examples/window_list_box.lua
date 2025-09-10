-- window

local items = {"Item 1", "Item 2", "Item 3"}
local current_item = 0

function draw()
    imgui.Begin("Lua UI")
    imgui.Text("Hello from Lua!")
    local changed, new_item = imgui.ListBox("Choose Item", current_item, items, 4)
    if changed then
        print("Combo selected index:", new_item)
        current_item = new_item
    end
    imgui.End()
end
