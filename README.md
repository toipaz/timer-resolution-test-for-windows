"# timer-resolution-test-for-windows"

	Commands:
		testapc [interval] [count] [PP] [TP]
		testsleep [type] [interval] [count] [PP] [TP]

	Parameters:
		**type**	test type (0 = sleep (default), 1 = timer api)

		**interval**	test interval [us], default is 1000.

		**count**	test iteration [number], default is 1000

		**pp**	process priority (0 = normal (default), 1 =high, 2 = REALTIME)

		**tp** 	thread priority (0 = normal (default), 1 = above normal, 2 = highest, 3 = time critical)

		ex) pp & tp
			priority	pp	tp
			"normal		0	0
						0	1
						0	2
						0	3
						1	0
						1	1
						1	2
						1	3
						2	0
						2	1
						2	2
			highest"	2	3