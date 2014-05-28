#ifndef ME_ERROR_H
#define ME_ERROR_H


#define ME_OK					0
#define ME_UNKNOWN				-1
#define ME_FILENOTFOUND			-2
#define ME_MEMORYLEAK			-3
#define ME_FILEOPENFAILED		-4
#define ME_FILEREADFAILED		-5
#define ME_FILEWRITEFAILED		-6
#define ME_PARSEXML				-7
#define ME_BADELEMENT			-8
#define ME_BADARG				-9

#define ME_FAILED(x)	(x!= ME_OK)


typedef int ME_RESULT;

#endif

