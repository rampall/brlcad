/* Generated by re2c */
#line 1 "condition_04.cg.re"

#line 5 "<stdout>"
{
	YYCTYPE yych;
	static void *yyctable[2] = {
		&&yyc_r1,
		&&yyc_r2,
	};
	goto *yyctable[YYGETCONDITION()];
/* *********************************** */
yyc_r1:

	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	if (yych <= '2') {
		if (yych <= '0') goto yy2;
		if (yych <= '1') goto yy5;
		goto yy3;
	} else {
		if (yych <= '`') goto yy2;
		if (yych <= 'a') goto yy7;
		if (yych <= 'b') goto yy9;
	}
yy2:
yy3:
	++YYCURSOR;
#line 4 "condition_04.cg.re"
	{ return "2"; }
#line 32 "<stdout>"
yy5:
	++YYCURSOR;
#line 3 "condition_04.cg.re"
	{ return "1"; }
#line 37 "<stdout>"
yy7:
	++YYCURSOR;
#line 5 "condition_04.cg.re"
	{ return "a"; }
#line 42 "<stdout>"
yy9:
	++YYCURSOR;
#line 6 "condition_04.cg.re"
	{ return "b"; }
#line 47 "<stdout>"
/* *********************************** */
yyc_r2:
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	if (yych <= '2') {
		if (yych <= '0') goto yy13;
		if (yych <= '1') goto yy16;
		goto yy14;
	} else {
		if (yych == 'b') goto yy18;
	}
yy13:
yy14:
	++YYCURSOR;
#line 4 "condition_04.cg.re"
	{ return "2"; }
#line 64 "<stdout>"
yy16:
	++YYCURSOR;
#line 3 "condition_04.cg.re"
	{ return "1"; }
#line 69 "<stdout>"
yy18:
	++YYCURSOR;
#line 6 "condition_04.cg.re"
	{ return "b"; }
#line 74 "<stdout>"
}
#line 8 "condition_04.cg.re"

