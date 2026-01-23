hub.init(function()
  ui.showvalue("Status", FORMAT_SIMPLE, 'Initialized')
  MOTOR_DIRECTION = 0
  hub.setmotorspeed(PORT1,0)
  hub.startthread('Main control loop', 4096, function()
    if gamepad.value(GAMEPAD1,GAMEPAD_LEFT_X) < 0 then
      if MOTOR_DIRECTION >= 0 then
        ui.showvalue("Direction", FORMAT_SIMPLE, 'Left')
        hub.setmotorspeed(PORT1,(-127))
        MOTOR_DIRECTION = -1
      end
    elseif gamepad.value(GAMEPAD1,GAMEPAD_LEFT_X) > 0 then
      if MOTOR_DIRECTION <= 0 then
        ui.showvalue("Direction", FORMAT_SIMPLE, 'Right')
        hub.setmotorspeed(PORT1,127)
        MOTOR_DIRECTION = 1
      end
    else
      if MOTOR_DIRECTION ~= 0 then
        ui.showvalue("Direction", FORMAT_SIMPLE, 'Stopped')
        hub.setmotorspeed(PORT1,0)
        MOTOR_DIRECTION = 0
      end
    end
    wait(20)
  end)
end)
