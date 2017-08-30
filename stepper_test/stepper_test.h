
/** Stepper cycle lifecycle */
int stepper_test_suite_lifecycle();

/** Stepper cycle timer period settings */
int stepper_test_suite_timer_period();

/** Stepper cycle timer period is aliquant part of motor step pulse delay */
int stepper_test_suite_timer_period_aliquant_step_delay();

/** Single motor: 3 steps tick by tick on max speed */
int stepper_test_suite_max_speed_tick_by_tick();

/** Single motor: 30000 steps on max speed */
int stepper_test_suite_max_speed_30000steps();

/** Single motor: 5 steps tick by tick on aliquant speed */
int stepper_test_suite_aliquant_speed_tick_by_tick();

/** 3 motors: draw triangle */
int stepper_test_suite_draw_triangle();

/** Small step delay error handlers */
int stepper_test_suite_small_step_delay_handlers();

/** Moving with variable speed: buffered steps tick by tick */
int stepper_test_suite_buffered_steps_tick_by_tick();

/** Moving with variable speed: buffered steps */
int stepper_test_suite_buffered_steps();

/** Step-dir driver std divider modes: 1/1, 1/18, 1/16, 1/32 */
int stepper_test_suite_driver_std_modes();

/** Step-dir driver std divider modes for 2 motors at a time */
int stepper_test_suite_driver_std_modes_2motors();

/** Single motor: exit bounds (issue #1) - whirl */
int stepper_test_suite_exit_bounds_issue1_whirl();

/** Single motor: exit bounds (issue #1) - steps */
int stepper_test_suite_exit_bounds_issue1_steps();

/** Single motor: exit bounds (issue #9) - steps */
int stepper_test_suite_exit_bounds_issue9_steps();

/** Single motor: test square signal (issue #16) */
int stepper_test_suite_square_sig_issue16();

///////

/** All tests in one bundle */
int stepper_test_suite();

