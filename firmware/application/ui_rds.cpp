/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
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

#include "ui_rds.hpp"

#include "portapack.hpp"
#include "baseband_api.hpp"
#include "portapack_shared_memory.hpp"

#include <cstring>

using namespace portapack;
using namespace rds;

namespace ui {

void RDSView::focus() {
	button_editpsn.focus();
}

void RDSView::on_tuning_frequency_changed(rf::Frequency f) {
	transmitter_model.set_tuning_frequency(f);
}

RDSView::~RDSView() {
	transmitter_model.disable();
	baseband::shutdown();
}

void RDSView::start_tx() {
	transmitter_model.set_sampling_rate(2280000U);
	transmitter_model.set_rf_amp(true);
	transmitter_model.set_lna(40);
	transmitter_model.set_vga(40);
	transmitter_model.set_baseband_bandwidth(1750000);
	transmitter_model.enable();
	
	baseband::set_rds_data(message_length);
}

void RDSView::paint(Painter&) {
	char RadioTextA[17];
	
	text_psn.set(PSN);
	
	memcpy(RadioTextA, RadioText, 16);
	RadioTextA[16] = 0;
	text_radiotexta.set(RadioTextA);
	memcpy(RadioTextA, RadioText + 16, 8);
	RadioTextA[8] = 0;
	text_radiotextb.set(RadioTextA);
}

RDSView::RDSView(NavigationView& nav) {
	baseband::run_image(portapack::spi_flash::image_tag_rds);
	
	strcpy(PSN, "TEST1234");
	strcpy(RadioText, "Radiotext test ABCD1234");
	
	add_children({
		&labels,
		&options_pty,
		&options_countrycode,
		&options_coverage,
		&options_tx,
		&check_mono_stereo,
		&check_TA,
		&check_TP,
		&check_MS,
		&sym_pi_code,
		&button_editpsn,
		&text_psn,
		&button_editradiotext,
		&text_radiotexta,
		&text_radiotextb,
		&tx_view,
	});
	
	tx_view.on_edit_frequency = [this, &nav]() {
		auto new_view = nav.push<FrequencyKeypadView>(receiver_model.tuning_frequency());
		new_view->on_changed = [this](rf::Frequency f) {
			receiver_model.set_tuning_frequency(f);
			this->on_tuning_frequency_changed(f);
		};
	};
	
	tx_view.on_start = [this]() {
		tx_view.set_transmitting(true);
		rds_flags.PI_code = sym_pi_code.value_hex_u64();
		rds_flags.PTY = options_pty.selected_index_value();
		rds_flags.DI = check_mono_stereo.value() ? 1 : 0;
		rds_flags.TP = check_TP.value();
		rds_flags.TA = check_TA.value();
		rds_flags.MS = check_MS.value();
		
		if (options_tx.selected_index() == 0)
			message_length = gen_PSN(PSN, &rds_flags);
		else if (options_tx.selected_index() == 1)
			message_length = gen_RadioText(RadioText, 0, &rds_flags);
		else
			message_length = gen_ClockTime(&rds_flags, 2016, 12, 1, 9, 23, 2);
		
		txing = true;
		start_tx();
	};
	
	tx_view.on_stop = [this]() {
		tx_view.set_transmitting(false);
		transmitter_model.disable();
		txing = false;
	};
	
	check_TA.set_value(true);
	check_TP.set_value(true);
	
	sym_pi_code.set_sym(0, 0xF);
	sym_pi_code.set_sym(1, 0x3);
	sym_pi_code.set_sym(2, 0xE);
	sym_pi_code.set_sym(3, 0x0);
	sym_pi_code.on_change = [this]() {
		rds_flags.PI_code = sym_pi_code.value_hex_u64();
	};
	
	options_pty.on_change = [this](size_t, int32_t v) {
		rds_flags.PTY = v;
	};
	options_pty.set_selected_index(0);				// None

	options_countrycode.on_change = [this](size_t, int32_t) {
		//rds_flags.PTY = v;
	};
	options_countrycode.set_selected_index(18);		// Baguette du fromage

	options_coverage.on_change = [this](size_t, int32_t) {
		//rds_flags.PTY = v;
	};
	options_coverage.set_selected_index(0);			// Local
	
	button_editpsn.on_select = [this, &nav](Button&) {
		textentry(nav, PSN, 8);
	};
	
	button_editradiotext.on_select = [this, &nav](Button&){
		textentry(nav, RadioText, 24);
	};
}

} /* namespace ui */
