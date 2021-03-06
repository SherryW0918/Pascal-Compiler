/* NAME: David Parker
   EID: dp24559
                    /*

/* codgen.c       Generate Assembly Code for x86         15 May 13   */

/* Copyright (c) 2013 Gordon S. Novak Jr. and The University of Texas at Austin
    */

/* Starter file for CS 375 Code Generation assignment.           */
/* Written by Gordon S. Novak Jr.                  */

/* This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License (file gpl.text) for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include "token.h"
#include "symtab.h"
#include "genasm.h"
#include "codegen.h"

void genc(TOKEN code);
/* RMIN -> RMAX (int  regs) FMIN -> FMAX (float registers) */
bool registers[FMAX + 1];

/* Set DEBUGGEN to 1 for debug printouts of code generation */
#define DEBUGGEN 0

int nextlabel;    /* Next available label number */
int stkframesize;   /* total stack frame size */

/* Top-level entry for code generator.
   pcode    = pointer to code:  (program foo (output) (progn ...))
   varsize  = size of local storage in bytes
   maxlabel = maximum label number used so far

Add this line to the end of your main program:
    gencode(parseresult, blockoffs[blocknumber], labelnumber);
The generated code is printed out; use a text editor to extract it for
your .s file.
         */

void gencode(TOKEN pcode, int varsize, int maxlabel)
  {  TOKEN name, code;
     name = pcode->operands;
     code = name->link->link;
     nextlabel = maxlabel + 1;
     stkframesize = asmentry(name->stringval,varsize);
     genc(code);
     asmexit(name->stringval);
  }

/* Get a register.   */
/* Need a type parameter or two versions for INTEGER or REAL */
int getreg(int kind)
  {
      /* Find if we need 32 bit or 64 bit register */
      /* Find first available register mark it used and return index */
    int i;
    if(kind == INTEGER || kind == BOOLETYPE || kind == POINTER) {
      for(i = RBASE; i <= RMAX; i++) {
        if(registers[i] == false) {
          registers[i] = true;
          return i;
        }
      }
      goto fail;
    }
    else if(kind == REAL || kind == STRINGTYPE) {
      for(i = FBASE; i <= FMAX; i++) {
        if(registers[i] == false) {
          registers[i] = true;
          return i;
        }
      }
      goto fail;
    }
    else {
      assert(false && "Oops! Wrong kind passed in, exiting program.");
    }
    fail:
      assert(false && "Oops! Out of registers, exiting program.");
  }

void freeReg(int reg) {
  if(!registers[reg]) {
    fprintf(stderr, "Trying to free an already free'd register. %s register #%d", ((reg <= RMAX) ? "INT" : "FP"), reg);
    assert(registers[reg]);
  }

  registers[reg] = false;
}

/* Trivial version */
/* Generate code for arithmetic expression, return a register number */
int genarith(TOKEN code) {   
  int num, reg,reg2;
  double fnum;
  TOKEN lhs,rhs;
  if (DEBUGGEN) { 
    printf("genarith\n");
    dbugprinttok(code);
  };

  switch (code->tokentype) {
    case NUMBERTOK:
      switch (code->datatype) {
        case INTEGER:
          num = code->intval;
          reg = getreg(INTEGER);
          if (num >= MINIMMEDIATE && num <= MAXIMMEDIATE)
            asmimmed(MOVL, num, reg);
          break;
        case REAL:
           fnum = code->realval;
           reg = getreg(REAL);
           makeflit(fnum,nextlabel);
           asmldflit(MOVSD, nextlabel++, reg);
          break;
        case POINTER:
          num = code->intval;
          reg = getreg(POINTER);
          asmimmed(MOVQ, num, reg);
          break;
      }
      break;
    case IDENTIFIERTOK:
    {
      SYMBOL sym = searchst(code->stringval);
      int offset = sym->offset - stkframesize;
      switch (code->datatype) { 
        case INTEGER:
          reg = getreg(INTEGER);
          asmld(MOVL, offset, reg, sym->namestring);
          break;
        case REAL:
          reg = getreg(REAL);
          asmld(MOVSD, offset, reg, sym->namestring);
          break;
        case POINTER:
          reg = getreg(POINTER);
          asmld(MOVQ, offset, reg, sym->namestring);
          break;
     };
    }
      break;
    case STRINGTOK:
      /* String constants always get moved into EDI (volatile register) */
        reg = getreg(STRINGTYPE);
        asmlitarg(nextlabel, EDI);
        makeblit(code->stringval, nextlabel++);
      break;
    case OPERATOR:
      lhs = code->operands;
      rhs = lhs->link;

      /* Funcalls do their own lhs and rhs recursion */
      if(code->whichval != FUNCALLOP) { 
        reg = genarith(lhs);

        if(rhs != NULL && !(lhs->whichval == POINTEROP && rhs->tokentype == NUMBERTOK))
          reg2 = genarith(rhs);
      }

      /* Add */
      if(code->whichval == PLUSOP) {
        switch(code->datatype) {
          case INTEGER:
            asmrr(ADDL, reg2, reg);
            break;
          case REAL:
            asmrr(ADDSD, reg2, reg);
            break;
        }
      }

      /* Subtract */
      else if(code->whichval == MINUSOP) {
        if(rhs == NULL) {
          /* Unary minus */
          switch(code->datatype) {
            case INTEGER:
              reg2 = getreg(INTEGER);
              asmimmed(MOVL, 0, reg2);
              asmrr(SUBL, reg, reg2);
              reg ^= reg2;
              reg2 ^= reg;
              reg ^= reg2;
              freeReg(reg2);
              break;
            case REAL:
              reg2 = getreg(REAL);
              asmfneg(reg,reg2);
              freeReg(reg2);
              break;
          }
        }
        else {
          switch(code->datatype) {
            case INTEGER:
              asmrr(SUBL, reg2, reg);
              break;
            case REAL:
              asmrr(SUBSD, reg2, reg);
              break;
          }
        }
      }

      /* Multiply */
      else if(code->whichval == TIMESOP) {
        switch(code->datatype) {
          case INTEGER:
            asmrr(IMULL, reg2, reg);
            break;
          case REAL:
            asmrr(MULSD, reg2, reg);
            break;
        }
      }

      /* Convert integer to a floating point number */
      else if(code->whichval == FLOATOP) {
        reg2 = getreg(REAL);
        /* Get a float register and move the integer value into it */
        asmfloat(reg, reg2);
        reg ^= reg2;
        reg2 ^= reg;
        reg ^= reg2;
        freeReg(reg2);
      }

      /* Convert a floating point number into an integer */
      else if(code->whichval == FIXOP) {
        reg2 = getreg(INTEGER);
        /* Get an integer register and move the float value into it */
        asmfix(reg,reg2);
        reg ^= reg2;
        reg2 ^= reg;
        reg ^= reg2;
        freeReg(reg2);
      }

      else if(code->whichval == FUNCALLOP) {
        /* Call genarith for our arguments */
        lhs = code->operands->link;
        reg = genarith(lhs);

        switch(lhs->datatype) {
          case INTEGER:
            asmrr(MOVL, reg, EDI);
            break;
        }

        /* TODO: Optimization for only loading if we return true from funcallin */
        if(lhs->datatype == REAL) {
            if(reg != XMM0 && registers[XMM0]) {
              asmsttemp(XMM0);
              asmrr(MOVSD,reg,XMM0);
              asmcall(code->operands->stringval);
              asmrr(MOVSD,XMM0,reg);
              asmldtemp(XMM0);
            }

            else if(reg != XMM0) {
              asmrr(MOVSD,reg,XMM0);
              freeReg(reg);
              asmcall(code->operands->stringval);
              registers[XMM0] = true;
              reg = XMM0;
            }

            else {
              asmcall(code->operands->stringval);
              registers[XMM0] = true;
              reg = XMM0;
            }
          }
        else {
          asmcall(code->operands->stringval);
        }
      }

      else if(code->whichval == AREFOP) {
        /* AREF needs to be loaded into a register from the given address */
        int offs = code->operands->link->intval;
        switch (code->datatype) {
          case INTEGER:
            reg2 = getreg(INTEGER);
            asmldr(MOVL, offs, reg, reg2, "^.");
            reg ^= reg2;
            reg2 ^= reg;
            reg ^= reg2;
            break;
          case REAL:
            reg2 = getreg(REAL);
            asmldr(MOVSD, offs, reg, reg2, "^.");
            reg ^= reg2;
            reg2 ^= reg;
            reg ^= reg2;
            break;
          case POINTER:
            reg2 = getreg(POINTER);
            asmldr(MOVQ, offs, reg, reg2, "^.");
            reg ^= reg2;
            reg2 ^= reg;
            reg ^= reg2;
            break;
       };
      }

      else if (code->whichval == POINTEROP) {
        /* NO OP */
        /* Pointer is loaded in AREF, since a pointer cannot exist without an AREF */
      }

      if(rhs != NULL && code->whichval != FUNCALLOP && code->whichval != POINTEROP)
        freeReg(reg2);
      break;
  };
  return reg;
}

/* Generate code for a Statement from an intermediate-code form */
void genc(TOKEN code) {  
 TOKEN tok, lhs, rhs;
 int reg, offs, reg2, extra;
 int lab1,lab2,lab3,lab4;
 SYMBOL sym;
 if(DEBUGGEN) { 
   printf("genc\n");
   dbugprinttok(code);
 };
 if(code->tokentype != OPERATOR) { 
   printf("Bad code token");
         dbugprinttok(code);
       };
 switch ( code->whichval ) { 

   case PROGNOP:
     tok = code->operands;
     while ( tok != NULL ) {  
       genc(tok);
       tok = tok->link;
     };
     break;

   case ASSIGNOP:      
     lhs = code->operands;
     rhs = lhs->link;
     reg = genarith(rhs);       

     /* LHS is an AREF and it's address is a pointer, call genarith for pointer op's */
     if(lhs->tokentype == OPERATOR && lhs->whichval == AREFOP && lhs->operands->tokentype != IDENTIFIERTOK) {
      reg2 = genarith(lhs->operands);
      sym = lhs->operands->symentry;
      offs = lhs->operands->link->intval;
      switch (rhs->datatype) { 
          case INTEGER:
            asmstr(MOVL, reg, offs, reg2, "^. ");
            break;
          case REAL:
            asmstr(MOVSD, reg, offs, reg2, "^. ");
            break;
          case POINTER:
            asmstr(MOVQ,reg, offs, reg2, "^. ");
            break;
       };
       if(reg != reg2)
        freeReg(reg2);
     }

     /* LHS is an AREF and it's address is an identifer with a variable offset, the offset is an expresion tree
        and genarith needs to be called on it, returning the offset into reg2    */
     else if(lhs->tokentype == OPERATOR && lhs->whichval == AREFOP && lhs->operands->tokentype == IDENTIFIERTOK &&
            lhs->operands->link->tokentype != NUMBERTOK){
          reg2 = genarith(lhs->operands->link);
          sym = lhs->operands->symentry;
          offs = sym->offset - stkframesize;

          switch (rhs->datatype) {          /* store value into lhs  */
          case INTEGER:
            asmop(CLTQ);
            asmstrr(MOVL, reg, offs, reg2, lhs->operands->stringval);
            break;
          case REAL:
            asmop(CLTQ);
            asmstrr(MOVSD, reg, offs, reg2, lhs->operands->stringval);
            break;
          case POINTER:
            asmop(CLTQ);
            asmstrr(MOVQ,reg, offs, reg2, lhs->operands->stringval);
            break;
       };
        if(reg != reg2)
          freeReg(reg2);
     }
      /* LHS is an AREF and it's address is an identifer is variable with a constant number for offset */
      /* Optimization Adds the offset for the array reference without doing another load */
     else if (lhs->tokentype == OPERATOR && lhs->whichval == AREFOP && lhs->operands->tokentype == IDENTIFIERTOK &&
              lhs->operands->link->tokentype == NUMBERTOK) {
        sym = lhs->operands->symentry;
        offs = sym->offset - stkframesize + lhs->operands->link->intval;
        switch (lhs->datatype) {          /* store value into lhs  */
          case INTEGER:
            asmst(MOVL, reg, offs, lhs->stringval);
            break;
          case REAL:
            asmst(MOVSD, reg, offs, lhs->stringval);
            break;
          case POINTER:
            asmst(MOVQ,reg, offs, lhs->stringval);
            break;
       };
     }

   /* Regular variable assignment */
   else {
        sym = lhs->symentry;
        offs = sym->offset - stkframesize;

       switch (lhs->datatype) {          /* store value into lhs  */
          case INTEGER:
            asmst(MOVL, reg, offs, lhs->stringval);
            break;
          case REAL:
            asmst(MOVSD, reg, offs, lhs->stringval);
            break;
          case POINTER:
            asmst(MOVQ,reg, offs, lhs->stringval);
            break;
       };
     }

     freeReg(reg);
     break;

    case LABELOP:
      asmlabel(code->operands->intval);
      break;

    case GOTOOP:
      asmjump(0, code->operands->intval);
      break;

    case IFOP:
      code = code->operands;
      lhs = code->operands;
      rhs = lhs->link;
      reg = genarith(lhs);
      reg2 = genarith(rhs);

      switch(lhs->datatype) {
        case INTEGER:
          asmrr(CMPL,reg2,reg);
          break;
        case REAL:
          asmrr(CMPSD,reg2,reg);
          break;
        case POINTER:
          asmrr(CMPQ,reg2,reg);
          break;
      }

      freeReg(reg);
      freeReg(reg2);

      lab1 = nextlabel++;
      asmjump(getop(code->whichval), lab1);

      /* Else part */
      if(code->link->link != NULL) {
        genc(code->link->link);
      }
      lab2 = nextlabel++;
      asmjump(0, lab2);

      asmlabel(lab1);

      genc(code->link);

      asmlabel(lab2);
      break;
    case FUNCALLOP:
      /* Call genarith for our arguments */
      tok = code->operands->link;
      reg = genarith(tok);

      switch(tok->datatype) {
        case INTEGER:
          asmrr(MOVL, reg, EDI);
          break;
      }

      freeReg(reg);

      asmcall(code->operands->stringval);
      break;
 };
}

/* Looks through all of the code tree to see if a function exists */
bool funcallin(TOKEN code) {
  if(code->tokentype == OPERATOR && code->whichval == FUNCALLOP) return true;
  if(code->operands != NULL) {
    if(funcallin(code->operands)) return true;
  }
  if(code->link != NULL) {
    if(funcallin(code->link)) return true;
  }

  return false;
}