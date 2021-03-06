/*
 * Berkeley Lab Checkpoint/Restart (BLCR) for Linux is Copyright (c)
 * 2004, The Regents of the University of California, through Lawrence
 * Berkeley National Laboratory (subject to receipt of any required
 * approvals from the U.S. Dept. of Energy).  All rights reserved.
 *
 * Portions may be copyrighted by others, as may be noted in specific
 * copyright notices within specific files.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Id: cr_arch.h,v 1.4 2008/09/02 22:48:36 phargrov Exp $
 */

// File for misc. arch-specific bits

#ifndef _CR_ARCH_H
#define _CR_ARCH_H        1

static inline struct pt_regs *
get_pt_regs(struct task_struct *tsk)
{
    return tsk->thread.regs;
}

#if 0 // Unused currenty, but might be needed again for trampolines
static inline unsigned long
get_stack_pointer(struct pt_regs *regs)
{
    return regs->gpr[1];
}
#endif

#endif
