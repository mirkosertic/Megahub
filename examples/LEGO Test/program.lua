hub.init(function()
  lego.selectmode(PORT1, 1)
end)

hub.main_control_loop(function()
  ui.showvalue("Dataset #0", FORMAT_SIMPLE, lego.getmodedataset(PORT1,0))
  wait(1000)
end)