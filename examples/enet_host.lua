-- window Slider

local window_open = true
local slider_value = 0.5
print("enet")
print(enet)

enet.initialize()
local host = enet.host_create({host = "127.0.0.1", port = 6789}, 32, 2, 0, 0)
if host then
    print("Host created")
end

function draw()
    if imgui.Begin("Lua ImGui Demo", window_open, {}) then
        imgui.Text("Hello from Lua!")
        imgui.Text("Server!")

    end
    imgui.End()
end

function cleanup()
    print("Lua cleanup called")
    enet.deinitialize()
end