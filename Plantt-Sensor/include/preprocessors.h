#ifndef PRERPOCESSERS_H
#define PRERPOCESSERS_H

#define PRINT_ENABLED true

#if PRINT_ENABLED == true
	#define PrintL(x) (Serial.println(x))
	#define PrintLn(x) (Serial.println(x))
	#define PrintF(x, y) (Serial.printf(x, y))
#else
	#define PrintLn(x)
	#define PrintF(x, y)
#endif

#endif //PRERPOCESSERS
