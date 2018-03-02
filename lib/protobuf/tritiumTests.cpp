extern "C"
{
  #include "dev_tritium.h"
}

#include "CppUTest/TestHarness.h"

TEST_GROUP(Tritium)
{
	void setup() {}
  void teardown() {}
};


TEST(Tritum, Tritium)
{
	drive_state.drive_mode = CRUISE_CONTROL;
  output = app_drive_drive(input);
  // check that output == (drive_state.KP * (drive_state.cruiseVelocity - average_velocity)) + drive_state.iTerm;
}