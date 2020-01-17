typedef struct
{
	tMotor port;
	int currPower;
} MotorInfo;

const float WIDTH = 0.6;
const float LENGTH = 0.7;
const int MAXIMUM_ANGLE = abs(atan((WIDTH/2.0)/LENGTH) * 180 / PI);
const int MAXIMUM_POWER_SHOOTING_WHEELS = -100;
const int MINIMUM_POWER_SHOOTING_WHEELS = -70;
const int ROTATABLE_WHEEL_POWER = 30;
const int PUSH_ROD_POWER = 50;
const int MAX_JAM_TIME = 1000;
const int ENC_LIMIT = 80;
const int ANGLE_TOL = 3;

TEV3Buttons get_button_press(bool hold)
{
	while (!getButtonPress(buttonAny))
	{}

	TEV3Buttons buttonSave = buttonNone;

	for (TEV3Buttons button = buttonUp; button <= buttonLeft && buttonSave == buttonNone; button++)
	{
		if (getButtonPress(button))
			buttonSave = button;
	}

	if (!hold)
	{
		while (getButtonPress(buttonAny))
		{}
	}

	return buttonSave;
}

void movePushRod(MotorInfo & pushRod, bool forward)
{
	nMotorEncoder[pushRod.port] = 0;

	if (forward)
		pushRod.currPower = PUSH_ROD_POWER;
	else
		pushRod.currPower = -PUSH_ROD_POWER;

	motor[pushRod.port] = pushRod.currPower;

	while (abs(nMotorEncoder[pushRod.port]) < ENC_LIMIT)
	{}

	pushRod.currPower = 0;
	motor[pushRod.port] = pushRod.currPower;
}

void adjustShootingAngle(MotorInfo & rotatableWheel)
{
	bool finishAdjustment = false;

	while (!finishAdjustment)
	{
		TEV3Buttons currentButton = get_button_press(true);

		if (currentButton == buttonLeft)
			rotatableWheel.currPower = -ROTATABLE_WHEEL_POWER;

		else if (currentButton == buttonRight)
			rotatableWheel.currPower = ROTATABLE_WHEEL_POWER;

		else if (currentButton == buttonEnter)
			finishAdjustment = true;

		motor[rotatableWheel.port] = rotatableWheel.currPower;

		while (getButtonPress(currentButton))
		{}

		rotatableWheel.currPower = 0;
		motor[rotatableWheel.port] = rotatableWheel.currPower;
	}
}

int createRandomAngle()
{
	int pos = random(1);

	if (pos == 0)
		return random(MAXIMUM_ANGLE);

	return random(MAXIMUM_ANGLE) * -1;
}

void createRandomMotorPower(MotorInfo & topWheel, MotorInfo & bottomWheel)
{
	topWheel.currPower = (random(30) * -1 + MINIMUM_POWER_SHOOTING_WHEELS);
	bottomWheel.currPower = (random(30) * -1 + MINIMUM_POWER_SHOOTING_WHEELS);
}

bool checkForJams(tSensors touchPort)
{
	time1[T1] = 0;

	while (SensorValue[touchPort] == 1 && time1[T1] <= MAX_JAM_TIME)
	{}

	return time1[T1] >= MAX_JAM_TIME;
}

void rotateWholeSystem(int angle, tSensors gyroPort, MotorInfo & rotatableWheel)
{
	rotatableWheel.currPower = ROTATABLE_WHEEL_POWER;

	if (getGyroDegrees(gyroPort) > angle)
		rotatableWheel.currPower *= -1;

	if (abs(getGyroDegrees(gyroPort) - angle) > ANGLE_TOL)
		motor[rotatableWheel.port] = rotatableWheel.currPower;
	else
		rotatableWheel.currPower = 0;

	if (rotatableWheel.currPower > 0)
  {
		while (getGyroDegrees(gyroPort) < angle)
		{}
	}
	else if (rotatableWheel.currPower < 0)
	{
		while (getGyroDegrees(gyroPort) > angle)
		{}
	}

	rotatableWheel.currPower = 0;
	motor[rotatableWheel.port] = rotatableWheel.currPower;
}

void setMotorPowerWheels(MotorInfo & topWheel, MotorInfo & bottomWheel, int topWheelPower, int bottomWheelPower)
{
	topWheel.currPower = topWheelPower;
	bottomWheel.currPower = bottomWheelPower;
	motor[topWheel.port] = topWheel.currPower;
	motor[bottomWheel.port] = bottomWheel.currPower;
}

void reloadSystem(MotorInfo & pushRod, tSensors touchPort, bool & empty)
{
	movePushRod(pushRod, true);

	if (SensorValue[touchPort] == 1)
	{
		movePushRod(pushRod, false);
		empty = checkForJams(touchPort);
	}
	else
		empty = true;
}

task main()
{
	tSensors gyroPort = S2;
	SensorType[gyroPort] = sensorEV3_Gyro;
	wait1Msec(50);
	SensorMode[gyroPort] = modeEV3Gyro_RateAndAngle;
	wait1Msec(50);

	while (getGyroRate(gyroPort) != 0)
	{}
	resetGyro(gyroPort);

	tSensors touchPort = S1;
	SensorType[touchPort] = sensorEV3_Touch;
	wait1Msec(50);

	MotorInfo topWheel;
	topWheel.port = motorA;
	topWheel.currPower = 0;

	MotorInfo bottomWheel;
	bottomWheel.port = motorD;
	bottomWheel.currPower = 0;

	MotorInfo pushRod;
	pushRod.port = motorB;
	pushRod.currPower = 0;

	MotorInfo rotatableWheel;
	rotatableWheel.port = motorC;
	rotatableWheel.currPower = 0;

	bool empty = false;

	displayString(2, "Press Enter for random shooting");
	displayString(4, "Press anywhere else for manual shooting");

	if (get_button_press(false) == buttonEnter)
	{
		setMotorPowerWheels(topWheel, bottomWheel, MINIMUM_POWER_SHOOTING_WHEELS, MINIMUM_POWER_SHOOTING_WHEELS);
		wait1Msec(5000);

		while (!empty)
		{
			rotateWholeSystem(createRandomAngle(), gyroPort, rotatableWheel);
			createRandomMotorPower(topWheel, bottomWheel);
			setMotorPowerWheels(topWheel, bottomWheel, topWheel.currPower, bottomWheel.currPower);
			reloadSystem(pushRod, touchPort, empty);
			wait1Msec(2000);
		}
		movePushRod(pushRod, false);
	}

	else
	{
		setMotorPowerWheels(topWheel, bottomWheel, MAXIMUM_POWER_SHOOTING_WHEELS, MAXIMUM_POWER_SHOOTING_WHEELS);
		eraseDisplay();
		displayString(2, "Use the left and right buttons to move system");
		displayString(4, "Press enter to shoot");

		while (!empty)
		{
			adjustShootingAngle(rotatableWheel);
			reloadSystem(pushRod, touchPort, empty);
		}
		movePushRod(pushRod, false);

	}

}
