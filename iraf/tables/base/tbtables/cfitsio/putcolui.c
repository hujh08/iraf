/*  This file, putcolui.c, contains routines that write data elements to    */
/*  a FITS image or table, with unsigned short datatype.                            */

/*  The FITSIO software was written by William Pence at the High Energy    */
/*  Astrophysic Science Archive Research Center (HEASARC) at the NASA      */
/*  Goddard Space Flight Center.                                           */

#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include "fitsio2.h"

/* declare variable for passing large firstelem values between routines */
extern OFF_T large_first_elem_val;

/*--------------------------------------------------------------------------*/
int ffpprui(fitsfile *fptr,  /* I - FITS file pointer                       */
            long  group,     /* I - group to write (1 = 1st group)          */
            long  firstelem, /* I - first vector element to write (1 = 1st) */
            long  nelem,     /* I - number of values to write               */
   unsigned short *array,    /* I - array of values that are written        */
            int  *status)    /* IO - error status                           */
/*
  Write an array of values to the primary array. Data conversion
  and scaling will be performed if necessary (e.g, if the datatype of
  the FITS array is not the same as the array being written).
*/
{
    long row;
    unsigned short nullvalue;

    /*
      the primary array is represented as a binary table:
      each group of the primary array is a row in the table,
      where the first column contains the group parameters
      and the second column contains the image itself.
    */

    if (fits_is_compressed_image(fptr, status))
    {
        /* this is a compressed image in a binary table */

        /* use the OFF_T variable to pass the first element value */
        if (firstelem != USE_LARGE_VALUE)
            large_first_elem_val = firstelem;

        fits_write_compressed_pixels(fptr, TUSHORT, large_first_elem_val, nelem,
            0, array, &nullvalue, status);
        return(*status);
    }

    row=maxvalue(1,group);

    ffpclui(fptr, 2, row, firstelem, nelem, array, status);
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffppnui(fitsfile *fptr,  /* I - FITS file pointer                       */
            long  group,     /* I - group to write(1 = 1st group)           */
            long  firstelem, /* I - first vector element to write(1 = 1st)  */
            long  nelem,     /* I - number of values to write               */
   unsigned short *array,    /* I - array of values that are written        */
   unsigned short nulval,    /* I - undefined pixel value                   */
            int  *status)    /* IO - error status                           */
/*
  Write an array of values to the primary array. Data conversion
  and scaling will be performed if necessary (e.g, if the datatype of the
  FITS array is not the same as the array being written).  Any array values
  that are equal to the value of nulval will be replaced with the null
  pixel value that is appropriate for this column.
*/
{
    long row;
    unsigned short nullvalue;

    /*
      the primary array is represented as a binary table:
      each group of the primary array is a row in the table,
      where the first column contains the group parameters
      and the second column contains the image itself.
    */

    if (fits_is_compressed_image(fptr, status))
    {
        /* this is a compressed image in a binary table */

        /* use the OFF_T variable to pass the first element value */
        if (firstelem != USE_LARGE_VALUE)
            large_first_elem_val = firstelem;

        nullvalue = nulval;  /* set local variable */
        fits_write_compressed_pixels(fptr, TUSHORT, large_first_elem_val, nelem,
            1, array, &nullvalue, status);
        return(*status);
    }

    row=maxvalue(1,group);

    ffpcnui(fptr, 2, row, firstelem, nelem, array, nulval, status);
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffp2dui(fitsfile *fptr,   /* I - FITS file pointer                     */
           long  group,      /* I - group to write(1 = 1st group)         */
           long  ncols,      /* I - number of pixels in each row of array */
           long  naxis1,     /* I - FITS image NAXIS1 value               */
           long  naxis2,     /* I - FITS image NAXIS2 value               */
  unsigned short *array,     /* I - array to be written                   */
           int  *status)     /* IO - error status                         */
/*
  Write an entire 2-D array of values to the primary array. Data conversion
  and scaling will be performed if necessary (e.g, if the datatype of the
  FITS array is not the same as the array being written).
*/
{
    /* call the 3D writing routine, with the 3rd dimension = 1 */

    ffp3dui(fptr, group, ncols, naxis2, naxis1, naxis2, 1, array, status);

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffp3dui(fitsfile *fptr,   /* I - FITS file pointer                     */
           long  group,      /* I - group to write(1 = 1st group)         */
           long  ncols,      /* I - number of pixels in each row of array */
           long  nrows,      /* I - number of rows in each plane of array */
           long  naxis1,     /* I - FITS image NAXIS1 value               */
           long  naxis2,     /* I - FITS image NAXIS2 value               */
           long  naxis3,     /* I - FITS image NAXIS3 value               */
  unsigned short *array,     /* I - array to be written                   */
           int  *status)     /* IO - error status                         */
/*
  Write an entire 3-D cube of values to the primary array. Data conversion
  and scaling will be performed if necessary (e.g, if the datatype of the
  FITS array is not the same as the array being written).
*/
{
    long tablerow, nfits, narray, ii, jj;
    long fpixel[3]= {1,1,1}, lpixel[3];
    /*
      the primary array is represented as a binary table:
      each group of the primary array is a row in the table,
      where the first column contains the group parameters
      and the second column contains the image itself.
    */
           
    if (fits_is_compressed_image(fptr, status))
    {
        /* this is a compressed image in a binary table */
        lpixel[0] = ncols;
        lpixel[1] = nrows;
        lpixel[2] = naxis3;
       
        fits_write_compressed_img(fptr, TUSHORT, fpixel, lpixel,
            0,  array, NULL, status);
    
        return(*status);
    }

    tablerow=maxvalue(1,group);

    if (ncols == naxis1 && nrows == naxis2)  /* arrays have same size? */
    {
      /* all the image pixels are contiguous, so write all at once */
      ffpclui(fptr, 2, tablerow, 1L, naxis1 * naxis2 * naxis3, array, status);
      return(*status);
    }

    if (ncols < naxis1 || nrows < naxis2)
       return(*status = BAD_DIMEN);

    nfits = 1;   /* next pixel in FITS image to write to */
    narray = 0;  /* next pixel in input array to be written */

    /* loop over naxis3 planes in the data cube */
    for (jj = 0; jj < naxis3; jj++)
    {
      /* loop over the naxis2 rows in the FITS image, */
      /* writing naxis1 pixels to each row            */

      for (ii = 0; ii < naxis2; ii++)
      {
       if (ffpclui(fptr, 2, tablerow, nfits, naxis1,&array[narray],status) > 0)
         return(*status);

       nfits += naxis1;
       narray += ncols;
      }
      narray += (nrows - naxis2) * ncols;
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffpssui(fitsfile *fptr,   /* I - FITS file pointer                       */
           long  group,      /* I - group to write(1 = 1st group)           */
           long  naxis,      /* I - number of data axes in array            */
           long  *naxes,     /* I - size of each FITS axis                  */
           long  *fpixel,    /* I - 1st pixel in each axis to write (1=1st) */
           long  *lpixel,    /* I - last pixel in each axis to write        */
  unsigned short *array,     /* I - array to be written                     */
           int  *status)     /* IO - error status                           */
/*
  Write a subsection of pixels to the primary array or image.
  A subsection is defined to be any contiguous rectangular
  array of pixels within the n-dimensional FITS data file.
  Data conversion and scaling will be performed if necessary 
  (e.g, if the datatype of the FITS array is not the same as
  the array being written).
*/
{
    long tablerow;
    long fpix[7], irange[7], dimen[7], astart, pstart;
    long off2, off3, off4, off5, off6, off7;
    long st10, st20, st30, st40, st50, st60, st70;
    long st1, st2, st3, st4, st5, st6, st7;
    long ii, i1, i2, i3, i4, i5, i6, i7;

    if (*status > 0)
        return(*status);

    if (fits_is_compressed_image(fptr, status))
    {
        /* this is a compressed image in a binary table */

        fits_write_compressed_img(fptr, TUSHORT, fpixel, lpixel,
            0,  array, NULL, status);
    
        return(*status);
    }

    if (naxis < 1 || naxis > 7)
      return(*status = BAD_DIMEN);

    tablerow=maxvalue(1,group);

     /* calculate the size and number of loops to perform in each dimension */
    for (ii = 0; ii < 7; ii++)
    {
      fpix[ii]=1;
      irange[ii]=1;
      dimen[ii]=1;
    }

    for (ii = 0; ii < naxis; ii++)
    {    
      fpix[ii]=fpixel[ii];
      irange[ii]=lpixel[ii]-fpixel[ii]+1;
      dimen[ii]=naxes[ii];
    }

    i1=irange[0];

    /* compute the pixel offset between each dimension */
    off2 =     dimen[0];
    off3 = off2 * dimen[1];
    off4 = off3 * dimen[2];
    off5 = off4 * dimen[3];
    off6 = off5 * dimen[4];
    off7 = off6 * dimen[5];

    st10 = fpix[0];
    st20 = (fpix[1] - 1) * off2;
    st30 = (fpix[2] - 1) * off3;
    st40 = (fpix[3] - 1) * off4;
    st50 = (fpix[4] - 1) * off5;
    st60 = (fpix[5] - 1) * off6;
    st70 = (fpix[6] - 1) * off7;

    /* store the initial offset in each dimension */
    st1 = st10;
    st2 = st20;
    st3 = st30;
    st4 = st40;
    st5 = st50;
    st6 = st60;
    st7 = st70;

    astart = 0;

    for (i7 = 0; i7 < irange[6]; i7++)
    {
     for (i6 = 0; i6 < irange[5]; i6++)
     {
      for (i5 = 0; i5 < irange[4]; i5++)
      {
       for (i4 = 0; i4 < irange[3]; i4++)
       {
        for (i3 = 0; i3 < irange[2]; i3++)
        {
         pstart = st1 + st2 + st3 + st4 + st5 + st6 + st7;
         for (i2 = 0; i2 < irange[1]; i2++)
         {
           if (ffpclui(fptr, 2, tablerow, pstart, i1, &array[astart],
              status) > 0)
              return(*status);

           astart += i1;
           pstart += off2;
         }
         st2 = st20;
         st3 = st3+off3;    
        }
        st3 = st30;
        st4 = st4+off4;
       }
       st4 = st40;
       st5 = st5+off5;
      }
      st5 = st50;
      st6 = st6+off6;
     }
     st6 = st60;
     st7 = st7+off7;
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffpgpui( fitsfile *fptr,   /* I - FITS file pointer                      */
            long  group,      /* I - group to write(1 = 1st group)          */
            long  firstelem,  /* I - first vector element to write(1 = 1st) */
            long  nelem,      /* I - number of values to write              */
   unsigned short *array,     /* I - array of values that are written       */
            int  *status)     /* IO - error status                          */
/*
  Write an array of group parameters to the primary array. Data conversion
  and scaling will be performed if necessary (e.g, if the datatype of
  the FITS array is not the same as the array being written).
*/
{
    long row;

    /*
      the primary array is represented as a binary table:
      each group of the primary array is a row in the table,
      where the first column contains the group parameters
      and the second column contains the image itself.
    */

    row=maxvalue(1,group);

    ffpclui(fptr, 1L, row, firstelem, nelem, array, status);
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffpclui( fitsfile *fptr,  /* I - FITS file pointer                       */
            int  colnum,     /* I - number of column to write (1 = 1st col) */
            long  firstrow,  /* I - first row to write (1 = 1st row)        */
            long  firstelem, /* I - first vector element to write (1 = 1st) */
            long  nelem,     /* I - number of values to write               */
   unsigned short *array,    /* I - array of values to write                */
            int  *status)    /* IO - error status                           */
/*
  Write an array of values to a column in the current FITS HDU.
  The column number may refer to a real column in an ASCII or binary table, 
  or it may refer to a virtual column in a 1 or more grouped FITS primary
  array.  FITSIO treats a primary array as a binary table with
  2 vector columns: the first column contains the group parameters (often
  with length = 0) and the second column contains the array of image pixels.
  Each row of the table represents a group in the case of multigroup FITS
  images.

  The input array of values will be converted to the datatype of the column 
  and will be inverse-scaled by the FITS TSCALn and TZEROn values if necessary.
*/
{
    int tcode, maxelem, hdutype;
    long twidth, incre, rownum, remain, next, ntodo;
    long tnull;
    OFF_T repeat, startpos, elemnum, large_elem, wrtptr, rowlen;
    double scale, zero;
    char tform[20], cform[20];
    char message[FLEN_ERRMSG];

    char snull[20];   /*  the FITS null value  */

    double cbuff[DBUFFSIZE / sizeof(double)]; /* align cbuff on word boundary */
    void *buffer;

    if (*status > 0)           /* inherit input status value if > 0 */
        return(*status);

    buffer = cbuff;

    if (firstelem == USE_LARGE_VALUE)
        large_elem = large_first_elem_val;
    else
        large_elem = firstelem;

    /*---------------------------------------------------*/
    /*  Check input and get parameters about the column: */
    /*---------------------------------------------------*/
    if (ffgcpr( fptr, colnum, firstrow, large_elem, nelem, 1, &scale, &zero,
        tform, &twidth, &tcode, &maxelem, &startpos,  &elemnum, &incre,
        &repeat, &rowlen, &hdutype, &tnull, snull, status) > 0)
        return(*status);

    if (tcode == TSTRING)   
         ffcfmt(tform, cform);     /* derive C format for writing strings */

    /*---------------------------------------------------------------------*/
    /*  Now write the pixels to the FITS column.                           */
    /*  First call the ffXXfYY routine to  (1) convert the datatype        */
    /*  if necessary, and (2) scale the values by the FITS TSCALn and      */
    /*  TZEROn linear scaling parameters into a temporary buffer.          */
    /*---------------------------------------------------------------------*/
    remain = nelem;           /* remaining number of values to write  */
    next = 0;                 /* next element in array to be written  */
    rownum = 0;               /* row number, relative to firstrow     */

    while (remain)
    {
        /* limit the number of pixels to process a one time to the number that
           will fit in the buffer space or to the number of pixels that remain
           in the current vector, which ever is smaller.
        */
        ntodo = minvalue(remain, maxelem);      
        ntodo = minvalue(ntodo, (repeat - elemnum));

        wrtptr = startpos + ((OFF_T)rownum * rowlen) + (elemnum * incre);

        ffmbyt(fptr, wrtptr, IGNORE_EOF, status); /* move to write position */

        switch (tcode) 
        {
            case (TSHORT):

              ffu2fi2(&array[next], ntodo, scale, zero,
                      (short *) buffer, status);
              ffpi2b(fptr, ntodo, incre, (short *) buffer, status);
              break;

            case (TLONGLONG):

                ffu2fi8(&array[next], ntodo, scale, zero,
                        (LONGLONG *) buffer, status);
                ffpi8b(fptr, ntodo, incre, (long *) buffer, status);
                break;

            case (TBYTE):
 
                ffu2fi1(&array[next], ntodo, scale, zero,
                        (unsigned char *) buffer, status);
                ffpi1b(fptr, ntodo, incre, (unsigned char *) buffer, status);
                break;

            case (TLONG):

                ffu2fi4(&array[next], ntodo, scale, zero,
                        (INT32BIT *) buffer, status);
                ffpi4b(fptr, ntodo, incre, (INT32BIT *) buffer, status);
                break;

            case (TFLOAT):

                ffu2fr4(&array[next], ntodo, scale, zero,
                        (float *) buffer, status);
                ffpr4b(fptr, ntodo, incre, (float *) buffer, status);
                break;

            case (TDOUBLE):
                ffu2fr8(&array[next], ntodo, scale, zero,
                        (double *) buffer, status);
                ffpr8b(fptr, ntodo, incre, (double *) buffer, status);
                break;

            case (TSTRING):  /* numerical column in an ASCII table */

                if (cform[1] != 's')  /*  "%s" format is a string */
                {
                  ffu2fstr(&array[next], ntodo, scale, zero, cform,
                          twidth, (char *) buffer, status);


                  if (incre == twidth)    /* contiguous bytes */
                     ffpbyt(fptr, ntodo * twidth, buffer, status);
                  else
                     ffpbytoff(fptr, twidth, ntodo, incre - twidth, buffer,
                            status);

                  break;
                }
                /* can't write to string column, so fall thru to default: */

            default:  /*  error trap  */
                sprintf(message, 
                    "Cannot write numbers to column %d which has format %s",
                      colnum,tform);
                ffpmsg(message);
                if (hdutype == ASCII_TBL)
                    return(*status = BAD_ATABLE_FORMAT);
                else
                    return(*status = BAD_BTABLE_FORMAT);

        } /* End of switch block */

        /*-------------------------*/
        /*  Check for fatal error  */
        /*-------------------------*/
        if (*status > 0)  /* test for error during previous write operation */
        {
         sprintf(message,
          "Error writing elements %ld thru %ld of input data array (ffpclui).",
             next+1, next+ntodo);
         ffpmsg(message);
         return(*status);
        }

        /*--------------------------------------------*/
        /*  increment the counters for the next loop  */
        /*--------------------------------------------*/
        remain -= ntodo;
        if (remain)
        {
            next += ntodo;
            elemnum += ntodo;
            if (elemnum == repeat)  /* completed a row; start on next row */
            {
                elemnum = 0;
                rownum++;
            }
        }
    }  /*  End of main while Loop  */


    /*--------------------------------*/
    /*  check for numerical overflow  */
    /*--------------------------------*/
    if (*status == OVERFLOW_ERR)
    {
       ffpmsg(
       "Numerical overflow during type conversion while writing FITS data.");
       *status = NUM_OVERFLOW;
    }

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffpcnui(fitsfile *fptr,  /* I - FITS file pointer                       */
            int  colnum,     /* I - number of column to write (1 = 1st col) */
            long  firstrow,  /* I - first row to write (1 = 1st row)        */
            long  firstelem, /* I - first vector element to write (1 = 1st) */
            long  nelem,     /* I - number of values to write               */
   unsigned short *array,    /* I - array of values to write                */
   unsigned short  nulvalue, /* I - value used to flag undefined pixels     */
            int  *status)    /* IO - error status                           */
/*
  Write an array of elements to the specified column of a table.  Any input
  pixels equal to the value of nulvalue will be replaced by the appropriate
  null value in the output FITS file. 

  The input array of values will be converted to the datatype of the column 
  and will be inverse-scaled by the FITS TSCALn and TZEROn values if necessary
*/
{
    tcolumn *colptr;
    long  ngood = 0, nbad = 0, ii, fstrow;
    OFF_T large_elem, repeat, first, fstelm;

    if (*status > 0)
        return(*status);

    /* reset position to the correct HDU if necessary */
    if (fptr->HDUposition != (fptr->Fptr)->curhdu)
    {
        ffmahd(fptr, (fptr->HDUposition) + 1, NULL, status);
    }
    else if ((fptr->Fptr)->datastart == DATA_UNDEFINED)
    {
        if ( ffrdef(fptr, status) > 0)               /* rescan header */
            return(*status);
    }

    colptr  = (fptr->Fptr)->tableptr;   /* point to first column */
    colptr += (colnum - 1);     /* offset to correct column structure */

    repeat = colptr->trepeat;  /* repeat count for this column */

    if (firstelem == USE_LARGE_VALUE)
        large_elem = large_first_elem_val;
    else
        large_elem = firstelem;

    /* hereafter, pass first element parameter via global variable */
    firstelem = USE_LARGE_VALUE;

    /* absolute element number in the column */
    first = (firstrow - 1) * repeat + large_elem;

    for (ii = 0; ii < nelem; ii++)
    {
      if (array[ii] != nulvalue)  /* is this a good pixel? */
      {
         if (nbad)  /* write previous string of bad pixels */
         {
            fstelm = ii - nbad + first;  /* absolute element number */
            fstrow = (fstelm - 1) / repeat + 1;  /* starting row number */
            fstelm = fstelm - (fstrow - 1) * repeat;  /* relative number */
            large_first_elem_val = fstelm;

            if (ffpclu(fptr, colnum, fstrow, firstelem, nbad, status) > 0)
                return(*status);

            nbad=0;
         }

         ngood = ngood +1;  /* the consecutive number of good pixels */
      }
      else
      {
         if (ngood)  /* write previous string of good pixels */
         {
            fstelm = ii - ngood + first;  /* absolute element number */
            fstrow = (fstelm - 1) / repeat + 1;  /* starting row number */
            fstelm = fstelm - (fstrow - 1) * repeat;  /* relative number */
            large_first_elem_val = fstelm;

            if (ffpclui(fptr, colnum, fstrow, firstelem,ngood, &array[ii-ngood],
                status) > 0)
                return(*status);

            ngood=0;
         }

         nbad = nbad +1;  /* the consecutive number of bad pixels */
      }
    }

    /* finished loop;  now just write the last set of pixels */

    if (ngood)  /* write last string of good pixels */
    {
      fstelm = ii - ngood + first;  /* absolute element number */
      fstrow = (fstelm - 1) / repeat + 1;  /* starting row number */
      fstelm = fstelm - (fstrow - 1) * repeat;  /* relative number */
      large_first_elem_val = fstelm;

      ffpclui(fptr, colnum, fstrow, firstelem, ngood, &array[ii-ngood], status);
    }
    else if (nbad) /* write last string of bad pixels */
    {
      fstelm = ii - nbad + first;  /* absolute element number */
      fstrow = (fstelm - 1) / repeat + 1;  /* starting row number */
      fstelm = fstelm - (fstrow - 1) * repeat;  /* relative number */
      large_first_elem_val = fstelm;

      ffpclu(fptr, colnum, fstrow, firstelem, nbad, status);
    }

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffu2fi1(unsigned short *input, /* I - array of values to be converted  */
            long ntodo,            /* I - number of elements in the array  */
            double scale,          /* I - FITS TSCALn or BSCALE value      */
            double zero,           /* I - FITS TZEROn or BZERO  value      */
            unsigned char *output, /* O - output array of converted values */
            int *status)           /* IO - error status                    */
/*
  Copy input to output prior to writing output to a FITS file.
  Do datatype conversion and scaling if required
*/
{
    long ii;
    double dvalue;

    if (scale == 1. && zero == 0.)
    {       
        for (ii = 0; ii < ntodo; ii++)
        {
            if (input[ii] > UCHAR_MAX)
            {
                *status = OVERFLOW_ERR;
                output[ii] = UCHAR_MAX;
            }
            else
                output[ii] = (unsigned char) input[ii];
        }
    }
    else
    {
        for (ii = 0; ii < ntodo; ii++)
        {
            dvalue = ((double) input[ii] - zero) / scale;

            if (dvalue < DUCHAR_MIN)
            {
                *status = OVERFLOW_ERR;
                output[ii] = 0;
            }
            else if (dvalue > DUCHAR_MAX)
            {
                *status = OVERFLOW_ERR;
                output[ii] = UCHAR_MAX;
            }
            else
                output[ii] = (unsigned char) (dvalue + .5);
        }
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffu2fi2(unsigned short *input, /* I - array of values to be converted */
            long ntodo,         /* I - number of elements in the array  */
            double scale,       /* I - FITS TSCALn or BSCALE value      */
            double zero,        /* I - FITS TZEROn or BZERO  value      */
            short *output,      /* O - output array of converted values */
            int *status)        /* IO - error status                    */
/*
  Copy input to output prior to writing output to a FITS file.
  Do datatype conversion and scaling if required
*/
{
    long ii;
    double dvalue;

    if (scale == 1. && zero == 32768.)
    {
        /* Instead of subtracting 32768, it is more efficient */
        /* to just flip the sign bit with the XOR operator */

        for (ii = 0; ii < ntodo; ii++)
             output[ii] =  ( *(short *) &input[ii] ) ^ 0x8000;
    }
    else if (scale == 1. && zero == 0.)
    {       
        for (ii = 0; ii < ntodo; ii++)
        {
            if (input[ii] > SHRT_MAX)
            {
                *status = OVERFLOW_ERR;
                output[ii] = SHRT_MAX;
            }
            else
                output[ii] = input[ii];
        }
    }
    else
    {
        for (ii = 0; ii < ntodo; ii++)
        {
            dvalue = ((double) input[ii] - zero) / scale;

            if (dvalue < DSHRT_MIN)
            {
                *status = OVERFLOW_ERR;
                output[ii] = SHRT_MIN;
            }
            else if (dvalue > DSHRT_MAX)
            {
                *status = OVERFLOW_ERR;
                output[ii] = SHRT_MAX;
            }
            else
            {
                if (dvalue >= 0)
                    output[ii] = (short) (dvalue + .5);
                else
                    output[ii] = (short) (dvalue - .5);
            }
        }
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffu2fi4(unsigned short *input, /* I - array of values to be converted */
            long ntodo,        /* I - number of elements in the array  */
            double scale,      /* I - FITS TSCALn or BSCALE value      */
            double zero,       /* I - FITS TZEROn or BZERO  value      */
            INT32BIT *output,      /* O - output array of converted values */
            int *status)       /* IO - error status                    */
/*
  Copy input to output prior to writing output to a FITS file.
  Do datatype conversion and scaling if required
*/
{
    long ii;
    double dvalue;

    if (scale == 1. && zero == 0.)
    {       
        for (ii = 0; ii < ntodo; ii++)
            output[ii] = (INT32BIT) input[ii];   /* copy input to output */
    }
    else
    {
        for (ii = 0; ii < ntodo; ii++)
        {
            dvalue = ((double) input[ii] - zero) / scale;

            if (dvalue < DINT_MIN)
            {
                *status = OVERFLOW_ERR;
                output[ii] = INT32_MIN;
            }
            else if (dvalue > DINT_MAX)
            {
                *status = OVERFLOW_ERR;
                output[ii] = INT32_MAX;
            }
            else
            {
                if (dvalue >= 0)
                    output[ii] = (INT32BIT) (dvalue + .5);
                else
                    output[ii] = (INT32BIT) (dvalue - .5);
            }
        }
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffu2fi8(unsigned short *input,  /* I - array of values to be converted  */
            long ntodo,             /* I - number of elements in the array  */
            double scale,           /* I - FITS TSCALn or BSCALE value      */
            double zero,            /* I - FITS TZEROn or BZERO  value      */
            LONGLONG *output,       /* O - output array of converted values */
            int *status)            /* IO - error status                    */
/*
  Copy input to output prior to writing output to a FITS file.
  Do datatype conversion and scaling if required
*/
{
#if (LONGSIZE == 32) && (! defined HAVE_LONGLONG)

/* don't have a native 8-byte integer, so have to construct the */
/* 2 equivalent 4-byte integers have the same bit pattern */

    unsigned long *uoutput;
    long ii, jj, kk, temp;
    double dvalue;

    uoutput = (unsigned long *) output;

#if BYTESWAPPED  /* jj points to the most significant part of the 8-byte int */
    jj = 1;
    kk = 0;
#else
    jj = 0;
    kk = 1;
#endif

    if (scale == 1. && zero == 0.)
    {       
        for (ii = 0; ii < ntodo; ii++, jj += 2, kk += 2)
        {
                output[jj] = 0;
                output[kk] = input[ii];
        }
    }
    else
    {
        for (ii = 0; ii < ntodo; ii++, jj += 2, kk += 2)
        {
            dvalue = (input[ii] - zero) / scale;

            if (dvalue < DLONGLONG_MIN)
            {
                *status = OVERFLOW_ERR;
                output[jj] = LONG_MIN;
                output[kk] = 0;
            }
            else if (dvalue > DLONGLONG_MAX)
            {
                *status = OVERFLOW_ERR;
                output[jj] = LONG_MAX;
                output[kk] = -1;
            }
            else
            {
                if (dvalue < 0)
                {
                   temp = (dvalue + 1.) / 4294967296. - 1.;
                   output[jj] = temp;
                   uoutput[kk] = 4294967296.  + 
                      (dvalue - (double) (temp + 1) * 4294967296.);
                }
                else
                {
                   temp = dvalue / 4294967296.;
                   output[jj] = temp;
                   uoutput[kk] = dvalue - (double) temp * 4294967296.;
                }
            }
        }
    }

#else

/* this is the much simpler case where the native 8-byte integer exists */

    long ii;
    double dvalue;

    if (scale == 1. && zero == 0.)
    {       
        for (ii = 0; ii < ntodo; ii++)
                output[ii] = input[ii];
    }
    else
    {
        for (ii = 0; ii < ntodo; ii++)
        {
            dvalue = (input[ii] - zero) / scale;

            if (dvalue < DLONGLONG_MIN)
            {
                *status = OVERFLOW_ERR;
                output[ii] = LONGLONG_MIN;
            }
            else if (dvalue > DLONGLONG_MAX)
            {
                *status = OVERFLOW_ERR;
                output[ii] = LONGLONG_MAX;
            }
            else
            {
                if (dvalue >= 0)
                    output[ii] = (LONGLONG) (dvalue + .5);
                else
                    output[ii] = (LONGLONG) (dvalue - .5);
            }
        }
    }

#endif

    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffu2fr4(unsigned short *input, /* I - array of values to be converted */
            long ntodo,        /* I - number of elements in the array  */
            double scale,      /* I - FITS TSCALn or BSCALE value      */
            double zero,       /* I - FITS TZEROn or BZERO  value      */
            float *output,     /* O - output array of converted values */
            int *status)       /* IO - error status                    */
/*
  Copy input to output prior to writing output to a FITS file.
  Do datatype conversion and scaling if required.
*/
{
    long ii;

    if (scale == 1. && zero == 0.)
    {       
        for (ii = 0; ii < ntodo; ii++)
                output[ii] = (float) input[ii];
    }
    else
    {
        for (ii = 0; ii < ntodo; ii++)
            output[ii] = ((double) input[ii] - zero) / scale;
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffu2fr8(unsigned short *input, /* I - array of values to be converted */
            long ntodo,        /* I - number of elements in the array  */
            double scale,      /* I - FITS TSCALn or BSCALE value      */
            double zero,       /* I - FITS TZEROn or BZERO  value      */
            double *output,    /* O - output array of converted values */
            int *status)       /* IO - error status                    */
/*
  Copy input to output prior to writing output to a FITS file.
  Do datatype conversion and scaling if required.
*/
{
    long ii;

    if (scale == 1. && zero == 0.)
    {       
        for (ii = 0; ii < ntodo; ii++)
                output[ii] = (double) input[ii];
    }
    else
    {
        for (ii = 0; ii < ntodo; ii++)
            output[ii] = ((double) input[ii] - zero) / scale;
    }
    return(*status);
}
/*--------------------------------------------------------------------------*/
int ffu2fstr(unsigned short *input, /* I - array of values to be converted  */
            long ntodo,        /* I - number of elements in the array  */
            double scale,      /* I - FITS TSCALn or BSCALE value      */
            double zero,       /* I - FITS TZEROn or BZERO  value      */
            char *cform,       /* I - format for output string values  */
            long twidth,       /* I - width of each field, in chars    */
            char *output,      /* O - output array of converted values */
            int *status)       /* IO - error status                    */
/*
  Copy input to output prior to writing output to a FITS file.
  Do scaling if required.
*/
{
    long ii;
    double dvalue;

    if (scale == 1. && zero == 0.)
    {       
        for (ii = 0; ii < ntodo; ii++)
        {
           sprintf(output, cform, (double) input[ii]);
           output += twidth;

           if (*output)  /* if this char != \0, then overflow occurred */
              *status = OVERFLOW_ERR;
        }
    }
    else
    {
        for (ii = 0; ii < ntodo; ii++)
        {
          dvalue = ((double) input[ii] - zero) / scale;
          sprintf(output, cform, dvalue);
          output += twidth;

          if (*output)  /* if this char != \0, then overflow occurred */
            *status = OVERFLOW_ERR;
        }
    }
    return(*status);
}