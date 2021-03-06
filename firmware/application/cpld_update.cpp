/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 *
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include "cpld_update.hpp"

#include "portapack_hal.hpp"

#include "jtag_target_gpio.hpp"
#include "cpld_max5.hpp"
#include "portapack_cpld_data.hpp"

bool cpld_update_if_necessary() {
	jtag::GPIOTarget target {
		portapack::gpio_cpld_tck,
		portapack::gpio_cpld_tms,
		portapack::gpio_cpld_tdi,
		portapack::gpio_cpld_tdo
	};
	jtag::JTAG jtag { target };
	cpld::max5::CPLD cpld { jtag };

	/* Unknown state */
	cpld.reset();
	cpld.run_test_idle();

	/* Run-Test/Idle */
	if( !cpld.idcode_ok() ) {
		return false;
	}

	/* Enter ISP:
	 * Ensures that the I/O pins transition smoothly from user mode to ISP
	 * mode. All pins are tri-stated.
	 */
	cpld.enter_isp();

	/* If silicon ID doesn't match, there's a serious problem. Leave CPLD
	 * in passive state.
	 */
	if( !cpld.silicon_id_ok() ) {
		return false;
	}

	/* Verify CPLD contents against current bitstream. */
	auto ok = cpld.verify(
		portapack::cpld::block_0,
		portapack::cpld::block_1
	);

	/* CPLD verifies incorrectly. Erase and program with current bitstream. */
	if( !ok ) {
		ok = cpld.program(
			portapack::cpld::block_0,
			portapack::cpld::block_1
		);
	}

	/* If programming OK, reset CPLD to user mode. Otherwise leave it in
	 * passive (ISP) state.
	 */
	if( ok ) {
		cpld.exit_isp();
	}

	return ok;
}
