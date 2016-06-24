#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define TRUE 1
#define FALSE 0

main (argc, argv)
int argc ;
char *argv[] ;
{
/* usage: hsplit files */

/* takes an unsplit Harwell/Boeing file - can be compressed or uncompressed */
/* and splits it into multiple files according to the name and type */

 int c, ignore, header, i, f, compressed , openned, len, nnames, j ;
#define S 200

 char line [S], name [S], *p, *q, *r, command [S], *dotpos,
	dir [S], outname [S], outname1 [S], h1 [S], h2 [S], h3 [S], h4 [S],
	names [S][S], unique ;

 FILE *in, *out ;

 for (f = 1 ; f < argc ; f++)
 {
	putchar ('\n') ;
	for (i = 1 ; i <= 80 ; i++) putchar ('=') ;
	printf ("\nprocessing %s\n", argv [f]) ;

	compressed = strchr (argv [f], 'Z') != (char *) NULL ;
	if (compressed)
	{
	   printf ("input file is compressed\n") ;

	   sprintf (command, "uncompress %s\n", argv [f]) ;
	   printf ("%s", command) ;
	   fflush (stdout) ;
	   system (command) ;

	   dotpos = strrchr (argv [f], '.') ;
	   *dotpos = '\0' ;
	}
	else
	{
	   printf ("input file is not compressed\n") ;
	}
	in = fopen (argv [f], "r") ;
	if (in == (FILE *) NULL)
	{ printf ("cannot open input file\n") ; exit (1) ; }

	sprintf (dir, "%s", argv [f]) ;
	dotpos = strchr (dir, '.') ;
	*dotpos = '\0' ;

	printf ("saving split files in subdirectory %s\n", dir) ;
	sprintf (command, "mkdir %s\n", dir) ;
	printf ("%s", command) ;
	fflush (stdout) ;
	system (command) ;

      nnames = 0 ;
      unique = 'b' ;

   /* read the input file and divide it */
   openned = FALSE ;
   while (fgets (line, 100, in) != (char *) NULL)
   {
      header = FALSE ;
      for (p = line ; *p ; p++)
      {
	 ignore = *p == 'E' || *p == 'D' || *p == 'e' || *p == 'd'
		|| isdigit (*p) || *p == '-' || isspace(*p) || *p == '.' 
		|| *p == '+' ;

	 if (!ignore) header = TRUE ;
      }

      if (header)
      {
	 /* HEADER LINE: */
	 /* set the name to the last nine characters of the first header line*/

	 strcpy (h1, line) ;
	 len = strlen (h1) ;
	 for (p = &(h1[72]) ; *p == ' ' ; p++) ;  /* skip leading blanks */
	 for (q = &h1 [len-1] ; !isgraph (*q) ; q--)
		/* skip trailing blanks,  newlines, etc. */
		*q = '\0' ;

	 for (r = p ; *r ; r++) /* change interior characters to underline */
	 {
	    	if (isspace (*r)) *r = '_' ;
		else if (isupper (*r)) *r = tolower (*r) ;
	 }

	 /* delete multiple underlines */
	 for (r = p ; *r ; r++)
	 {
	    while (*r == '_' && *(r+1) == '_')
	    {
	       for (q = r+1 ; *q ; q++) *(q-1) = *q ;
	       *(q-1) = *q ;
	    }
	 }

	 /* LINE WITH NUMBER OF LINES IN MATRIX DESCRIPTION */
	 fgets (h2, 100, in) ;

	 /* LINE WITH TYPE AND MATRIX DIMENSION */
	 fgets (h3, 100, in) ;

	 /* open the new file */
	 sprintf (outname1, "%s/%s.%c%c%c", dir, p,
		tolower (h3[0]), tolower (h3[1]), tolower (h3[2])) ;

	 /* make sure the name is unique */
	 for (i = 1 ; i <= nnames ; i++)
	 {
	    if (strcmp (names [i], outname1) == 0)
	    {
	       printf ("Name conflict: %s already exists\n", outname1) ;
	       sprintf (outname1, "%s/%s%c.%c%c%c", dir, p, unique++,
			tolower (h3[0]), tolower (h3[1]), tolower (h3[2])) ;
	    }
	 }
	 nnames++ ;
	 printf ("%s %d\n", outname1, nnames) ;
	 strcpy (names [nnames], outname1) ;
	 if (openned) fclose (out) ;
	 out = fopen (outname1, "w") ;
	 if (out == (FILE *) NULL) { printf ("cannot open\n") ; exit (1) ; }
	 openned = TRUE ;

	 /* write the first three header lines */
	 fputs (line, out) ;
	 fputs (h2, out) ;
	 fputs (h3, out) ;

	 /* LINE WITH FORMAT INFO */
	 fgets (line, 100, in) ;
	 fputs (line, out) ;

	 /* POSSIBLY A LINE WITH OTHER HEADER INFO STUFF */
	 fgets (line, 100, in) ;
	 fputs (line, out) ;
      }
      else
      {
	 if (openned) fputs (line, out) ;
      }
	    
   }
   if (openned) fclose (out) ;
   openned = FALSE ;

   fclose (in) ;

   /* create a short shell script which will re-constitute the input file */
   sprintf (outname1, "%s.cat", dir) ;
   out = fopen (outname1, "w") ;
   fprintf (out, "# Use this to re-constitute the input file %s.data\n", dir) ;
   if (compressed) fprintf (out, "z") ;
   fprintf (out, "cat \\\n") ;

   for (i = 1 ; i <= nnames ; i++)
   {
	fprintf (out, "\"%s", names [i]) ;
	if (compressed) fprintf (out,".Z") ;
	fprintf (out,"\"	\\\n") ;

        /* get headers of all files created */
	putchar ('\n') ;
	for (j = 1 ; j <= 80 ; j++) putchar ('-') ;
        sprintf (command, "head -6 \"%s\"\n", names [i]) ;
        printf ("\n%s", command) ;
        fflush (stdout) ;
        system (command) ;
   }

   fprintf (out, "> %s.recon\n", dir) ;
   fclose (out) ;
   
   /* compress the split files if the input is compressed */
   if (compressed)
   {
	sprintf (command, "compress -f %s/*\n", dir) ;
	printf ("%s", command) ;
	fflush (stdout) ;
	system (command) ;
   }

   /* re-constitute the split output  files and compare with the input */
   sprintf (command,
   "chmod +x %s.cat ; %s.cat ; diff %s %s.recon | wc ; rm %s.recon\n",
	dir, dir, argv [f], dir, dir) ;
   printf ("%s", command) ;
   fflush (stdout) ;
   system (command) ;

   fflush (stdout) ;

   if (compressed)
   {
      /* re-compress input file */
      sprintf (command, "compress %s\n", argv [f]) ;
      printf ("%s", command) ;
      fflush (stdout) ;
      system (command) ;
   }
 }
} 
