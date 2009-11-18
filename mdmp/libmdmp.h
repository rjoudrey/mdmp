/*
    libMDMP - main MDmp library - header file
    Copyright (c) 2009 Vlad-Ioan Topan

    author:           Vlad-Ioan Topan (vtopan / gmail.com)
    file version:     0.1 (ALPHA)
    web:              http://code.google.com/p/mdmp/

    This file is part of MDmp.

    MDmp is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef LIBMDMP_H
#define LIBMDMP_H

//#include <windows.h>

//=== macros =================================================================//
#define ALIGN_ADDR(addr, alignment) (addr % alignment) ? (addr + alignment - (addr % alignment)) : (addr)

//=== constants ==============================================================//
#define SE_DEBUG_PRIVILEGE 20

//=== data types =============================================================//

//=== APIs ===================================================================//
int __stdcall initMDMP(); // libMDmp initialization function; returns 1 on success, 0 on fail

#endif // LIBMDMP_H