/*
 * pid_controller.cpp
 *
 *  Created on: Nov 11, 2009
 *      Author: dc
 */


#include <cmath>
#include <libconfig.h++>
#include <gtest/gtest.h>

#include <barrett/math/vector.h>
#include <barrett/systems/helpers.h>
#include <barrett/systems/manual_execution_manager.h>
#include <barrett/systems/abstract/system.h>
#include <barrett/systems/constant.h>
#include <barrett/systems/pid_controller.h>

#include "./exposed_io_system.h"


namespace {
using namespace barrett;


typedef math::Vector<5> i_type;
const double T_s = 0.002;


class PIDControllerTest : public ::testing::Test {
public:
	PIDControllerTest() :
		pid(), feedbackSignal(i_type()), eios(), a()
	{
		pid.setSamplePeriod(T_s);
		systems::connect(feedbackSignal.output, pid.feedbackInput);
		systems::connect(eios.output, pid.referenceInput);
		systems::connect(pid.controlOutput, eios.input);
	}

protected:
	systems::PIDController<i_type, i_type> pid;
	systems::Constant<i_type> feedbackSignal;
	ExposedIOSystem<i_type> eios;
	i_type a;
};

TEST_F(PIDControllerTest, SamplePeriodZeroInitilized) {
	systems::PIDController<i_type, i_type> pid;
	EXPECT_EQ(0.0, pid.getSamplePeriod());
}

TEST_F(PIDControllerTest, SamplePeriodMatchesDefaultExecutionManager) {
	systems::ManualExecutionManager mem(5.8);
	systems::System::defaultExecutionManager = &mem;
	systems::PIDController<i_type, i_type> pid2;
	EXPECT_EQ(5.8, pid2.getSamplePeriod());
}

TEST_F(PIDControllerTest, SamplePeriodChangesWithExecutionManager) {
	systems::PIDController<i_type, i_type> pid2;
	EXPECT_EQ(0.0, pid2.getSamplePeriod());

	systems::ManualExecutionManager mem(5.8);
	pid2.setExecutionManager(&mem);
	EXPECT_EQ(5.8, pid2.getSamplePeriod());

	pid2.setExecutionManager(NULL);
	EXPECT_EQ(0.0, pid2.getSamplePeriod());
}

TEST_F(PIDControllerTest, GainsZeroInitilized) {
	// kp
	a.setConstant(1);
	eios.setOutputValue(a);
	EXPECT_EQ(i_type(), eios.getInputValue());

	// ki
	a.setConstant(1);
	eios.setOutputValue(a);
	EXPECT_EQ(i_type(), eios.getInputValue());

	// kd
	a.setConstant(1e3);
	eios.setOutputValue(a);
	EXPECT_EQ(i_type(), eios.getInputValue());
}

TEST_F(PIDControllerTest, ConfigCtor) {
	libconfig::Config config;
	config.readFile("test.config");
	systems::PIDController<i_type, i_type> pid2(config.lookup("pid_controller_test"));

	a.setConstant(1);
	EXPECT_EQ(a, pid2.getKp());

	a.setConstant(2);
	EXPECT_EQ(a, pid2.getKi());

	a.setConstant(3);
	EXPECT_EQ(a, pid2.getKd());

	a.setConstant(4);
	EXPECT_EQ(a, pid2.getIntegratorLimit());

	a.setConstant(5);
	EXPECT_EQ(a, pid2.getControlSignalLimit());
}

TEST_F(PIDControllerTest, SetFromConfig) {
	libconfig::Config config;
	config.readFile("test.config");
	pid.setFromConfig(config.lookup("pid_controller_test"));

	a.setConstant(1);
	EXPECT_EQ(a, pid.getKp());

	a.setConstant(2);
	EXPECT_EQ(a, pid.getKi());

	a.setConstant(3);
	EXPECT_EQ(a, pid.getKd());

	a.setConstant(4);
	EXPECT_EQ(a, pid.getIntegratorLimit());

	a.setConstant(5);
	EXPECT_EQ(a, pid.getControlSignalLimit());
}

TEST_F(PIDControllerTest, SetKp) {
	a.setConstant(38);
	pid.setKp(a);

	for (size_t i = 0; i < 10; ++i) {
		eios.setOutputValue(a);
		EXPECT_EQ(a*a, eios.getInputValue());
	}
}

TEST_F(PIDControllerTest, SetKi) {
	a.setConstant(500);
	pid.setKi(a);

	a.setConstant(1);
	for (size_t i = 0; i < 10; ++i) {
		eios.setOutputValue(a);
		EXPECT_EQ(a*i, eios.getInputValue());
	}
}

TEST_F(PIDControllerTest, SetKd) {
	a.setConstant(10*T_s);
	pid.setKd(a);

	size_t i = 0;
	a.setConstant(i);
	eios.setOutputValue(a);
	EXPECT_EQ(i_type(0.0), eios.getInputValue());
	for (i = 1; i < 10; ++i) {
		a.setConstant(i);
		eios.setOutputValue(a);
		EXPECT_EQ(i_type(10), eios.getInputValue());
	}
}

TEST_F(PIDControllerTest, SetIntegratorState) {
	a.setConstant(10);
	pid.setKi(a);

	a.setConstant(1);
	pid.setIntegratorState(a);

	a.setConstant(0);
	for (size_t i = 0; i < 10; ++i) {
		eios.setOutputValue(a);
		EXPECT_EQ(i_type(1), eios.getInputValue());
	}
}

TEST_F(PIDControllerTest, SetIntegratorLimit) {
	a.setConstant(500);
	pid.setKi(a);

	a.setConstant(5.8);
	pid.setIntegratorLimit(a);

	a.setConstant(1);
	for (size_t i = 0; i <= 5; ++i) {
		eios.setOutputValue(a);
		EXPECT_EQ(a*i, eios.getInputValue());
	}
	for (size_t i = 6; i < 10; ++i) {
		eios.setOutputValue(a);
		EXPECT_EQ(i_type(5.8), eios.getInputValue());
	}

	a.setConstant(-1);
	for (size_t i = 0; i <= 11; ++i) {
		eios.setOutputValue(a);
		EXPECT_EQ(5.8 + (a*i).cwise(), eios.getInputValue());
	}
	for (size_t i = 12; i < 15; ++i) {
		eios.setOutputValue(a);
		EXPECT_EQ(i_type(-5.8), eios.getInputValue());
	}

	// the output isn't saturated, just the integrator
	a.setConstant(500);
	pid.setKp(a);

	a.setConstant(-1);
	eios.setOutputValue(a);
	EXPECT_EQ(i_type(-505.8), eios.getInputValue());
}

TEST_F(PIDControllerTest, SetControlSignalLimit) {
	a.setConstant(10);
	pid.setKp(a);
	pid.setControlSignalLimit(a);

	eios.setOutputValue(i_type(0.1));
	EXPECT_EQ(i_type(0.1) * a, eios.getInputValue());
	eios.setOutputValue(i_type(2.0));
	EXPECT_EQ(a, eios.getInputValue());
	eios.setOutputValue(i_type(-5));
	EXPECT_EQ(-a, eios.getInputValue());
}

TEST_F(PIDControllerTest, ResetIntegrator) {
	pid.setKi(i_type(500));

	a.setConstant(1);
	for (size_t i = 0; i < 10; ++i) {
		eios.setOutputValue(a);
		EXPECT_EQ(a*i, eios.getInputValue());
	}

	pid.resetIntegrator();

	// start at i=1 because (this time through) the previous input sample
	// wasn't zero
	for (size_t i = 1; i < 10; ++i) {
		eios.setOutputValue(a);
		EXPECT_EQ(a*i, eios.getInputValue());
	}
}


}
