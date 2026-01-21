hub.init(function()
  ui.showvalue("Status", FORMAT_SIMPLE, 'Initialized')
end)

hub.main_control_loop(function()
  if gamepad.value(GAMEPAD1,GAMEPAD_LEFT_X) > 0 then
    if not MOTOR_RUNNING then
      hub.setmotorspeed(PORT1,(-127))
      ui.showvalue("Port 1", FORMAT_SIMPLE, 'Turning left')
      MOTOR_RUNNING = true
    end
  else
    if gamepad.value(GAMEPAD1,GAMEPAD_LEFT_X) < 0 then
      if not MOTOR_RUNNING then
        hub.setmotorspeed(PORT1,128)
        ui.showvalue("Port 1", FORMAT_SIMPLE, 'Turning right')
        MOTOR_RUNNING = true
      end
    else
      if MOTOR_RUNNING then
        ui.showvalue("Port 1", FORMAT_SIMPLE, 'Off')
        MOTOR_RUNNING = false
      end
    end
  end
end)