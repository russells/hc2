#ifndef time_h_INCLUDED
#define time_h_INCLUDED

struct Time {
	char ht;		/** Hour tens */
	char h1;		/** Hour units */
	char mt;		/** Minute tens */
	char m1;		/** Minute units */
	uint8_t seconds;	/** Count of seconds within a minute */
};

#endif
