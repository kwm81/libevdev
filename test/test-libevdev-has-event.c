/*
 * Copyright © 2013 Red Hat, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */

#include <config.h>
#include <linux/input.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>

#include "test-common.h"

static int evbits[] = {
	EV_SYN, EV_KEY, EV_REL, EV_ABS, EV_MSC,
	EV_SW, EV_LED, EV_SND, EV_FF,
	/* Intentionally skipping these, they're different
	 * EV_PWR, EV_FF_STATUS, EV_REP, */
	-1,
};

START_TEST(test_has_ev_bit)
{
	int *evbit = evbits;

	while(*evbit != -1) {
		struct uinput_device* uidev;
		struct libevdev *dev;
		int i, rc;

		rc = test_create_device(&uidev, &dev,
					*evbit, 0,
					-1);
		ck_assert_msg(rc == 0, "%s: Failed to create device with: %s",
				libevdev_get_event_type_name(*evbit),
				strerror(-rc));

		ck_assert_msg(libevdev_has_event_type(dev, EV_SYN), "for event type %d\n", *evbit);
		ck_assert_msg(libevdev_has_event_type(dev, *evbit), "for event type %d\n", *evbit);

		for (i = 0; i <= EV_MAX; i++) {
			if (i == EV_SYN || i == *evbit)
				continue;

			ck_assert_msg(!libevdev_has_event_type(dev, i), "for event type %d\n", i);
		}

		libevdev_free(dev);
		uinput_device_free(uidev);

		evbit++;
	}
}
END_TEST

START_TEST(test_ev_bit_limits)
{
	int *evbit = evbits;

	while(*evbit != -1) {
		struct uinput_device* uidev;
		struct libevdev *dev;
		int rc;

		rc = test_create_device(&uidev, &dev,
					*evbit, 0,
					-1);
		ck_assert_msg(rc == 0, "Failed to create device: %s", strerror(-rc));

		ck_assert_int_eq(libevdev_has_event_type(dev, EV_MAX + 1), 0);
		ck_assert_int_eq(libevdev_has_event_type(dev, INT_MAX), 0);
		ck_assert_int_eq(libevdev_has_event_type(dev, UINT_MAX), 0);

		libevdev_free(dev);
		uinput_device_free(uidev);

		evbit++;
	}
}
END_TEST

START_TEST(test_event_codes)
{
	int *evbit = evbits;

	while(*evbit != -1) {
		struct uinput_device* uidev;
		struct libevdev *dev;
		int rc;
		int code, max;
		if (*evbit == EV_SYN) {
			evbit++;
			continue;
		}

		max = libevdev_get_event_type_max(*evbit);

		for (code = 1; code < max; code += 10) {
			rc = test_create_device(&uidev, &dev,
						*evbit, code,
						-1);
			ck_assert_msg(rc == 0, "Failed to create device: %s", strerror(-rc));

			ck_assert_msg(libevdev_has_event_type(dev, *evbit), "for event type %d\n", *evbit);
			ck_assert_msg(libevdev_has_event_code(dev, *evbit, code), "for type %d code %d", *evbit, code);
			ck_assert_msg(libevdev_has_event_code(dev, EV_SYN, SYN_REPORT), "for EV_SYN");
			/* always false */
			ck_assert_msg(!libevdev_has_event_code(dev, EV_PWR, 0), "for EV_PWR");

			libevdev_free(dev);
			uinput_device_free(uidev);
		}

		evbit++;
	}
}
END_TEST

START_TEST(test_event_code_limits)
{
	int *evbit = evbits;

	while(*evbit != -1) {
		struct uinput_device* uidev;
		struct libevdev *dev;
		int rc;
		int max;

		if (*evbit == EV_SYN) {
			evbit++;
			continue;
		}

		max = libevdev_get_event_type_max(*evbit);
		ck_assert(max != -1);

		rc = test_create_device(&uidev, &dev,
					*evbit, 1,
					-1);
		ck_assert_msg(rc == 0, "Failed to create device: %s", strerror(-rc));

		ck_assert_msg(!libevdev_has_event_code(dev, *evbit, max), "for type %d code %d", *evbit, max);
		ck_assert_msg(!libevdev_has_event_code(dev, *evbit, INT_MAX), "for type %d code %d", *evbit, INT_MAX);
		ck_assert_msg(!libevdev_has_event_code(dev, *evbit, UINT_MAX), "for type %d code %d", *evbit, UINT_MAX);

		libevdev_free(dev);
		uinput_device_free(uidev);

		evbit++;
	}
}
END_TEST

START_TEST(test_ev_rep)
{
	struct uinput_device* uidev;
	int rc;

	/* EV_REP is special, it's always fully set if set at all,
	   can't test this through uinput though */
	rc = uinput_device_new_with_events(&uidev, "test device", DEFAULT_IDS,
					   EV_REP, 0,
					   -1);
	ck_assert_int_eq(rc, -EINVAL);
}
END_TEST

START_TEST(test_ev_rep_values)
{
	struct uinput_device* uidev;
	struct libevdev *dev;
	int rc;
	int delay = 0xab, period = 0xbc;

	/* EV_REP is special, it's always fully set if set at all, can't set
	   it through uinput though. */
	rc = test_create_device(&uidev, &dev, -1);
	ck_assert_msg(rc == 0, "Failed to create device: %s", strerror(-rc));

	ck_assert_int_eq(libevdev_get_repeat(dev, NULL, NULL), -1);
	ck_assert_int_eq(libevdev_get_repeat(dev, &delay, NULL), -1);
	ck_assert_int_eq(libevdev_get_repeat(dev, NULL, &period), -1);
	ck_assert_int_eq(libevdev_get_repeat(dev, &delay, &period), -1);

	ck_assert_int_eq(delay, 0xab);
	ck_assert_int_eq(period, 0xbc);

	uinput_device_free(uidev);
	libevdev_free(dev);
}
END_TEST

START_TEST(test_input_props)
{
	struct uinput_device* uidev;
	struct libevdev *dev;
	int rc;

	rc = test_create_device(&uidev, &dev,
				EV_ABS, ABS_X,
				-1);
	ck_assert_msg(rc == 0, "Failed to create device: %s", strerror(-rc));

	ck_assert_int_eq(libevdev_has_property(dev, INPUT_PROP_MAX + 1), 0);
	ck_assert_int_eq(libevdev_has_property(dev, INPUT_PROP_MAX), 0);
	ck_assert_int_eq(libevdev_has_property(dev, INPUT_PROP_BUTTONPAD), 0);
	/* FIXME: no idea how to set props on uinput devices */

	uinput_device_free(uidev);
	libevdev_free(dev);
}
END_TEST

START_TEST(test_no_slots)
{
	struct uinput_device* uidev;
	struct libevdev *dev;
	const char *str;
	int rc;

	dev = libevdev_new();

	str = libevdev_get_name(dev);
	ck_assert(str != NULL);
	ck_assert_int_eq(strlen(str), 0);

	rc = test_create_device(&uidev, &dev,
				EV_ABS, ABS_X,
				EV_ABS, ABS_Y,
				EV_ABS, ABS_MT_POSITION_X,
				EV_ABS, ABS_MT_POSITION_Y,
				-1);
	ck_assert_msg(rc == 0, "Failed to create device: %s", strerror(-rc));

	ck_assert_int_eq(libevdev_get_num_slots(dev), -1);
	ck_assert_int_eq(libevdev_get_current_slot(dev), -1);

	uinput_device_free(uidev);
	libevdev_free(dev);
}
END_TEST

START_TEST(test_slot_number)
{
	struct uinput_device* uidev;
	struct libevdev *dev;
	const char *str;
	int rc;

	dev = libevdev_new();

	str = libevdev_get_name(dev);
	ck_assert(str != NULL);
	ck_assert_int_eq(strlen(str), 0);

	rc = test_create_device(&uidev, &dev,
				EV_ABS, ABS_X,
				EV_ABS, ABS_Y,
				EV_ABS, ABS_MT_POSITION_X,
				EV_ABS, ABS_MT_POSITION_Y,
				EV_ABS, ABS_MT_SLOT,
				-1);
	ck_assert_msg(rc == 0, "Failed to uinput device: %s", strerror(-rc));

	ck_assert_int_eq(libevdev_get_num_slots(dev), 1);
	ck_assert_int_eq(libevdev_get_current_slot(dev), 0);

	uinput_device_free(uidev);
	libevdev_free(dev);
}
END_TEST


START_TEST(test_device_name)
{
	struct uinput_device* uidev;
	struct libevdev *dev;
	struct input_id ids = {1, 2, 3, 4};
	const char *str;
	int rc;

	dev = libevdev_new();

	str = libevdev_get_name(dev);
	ck_assert(str != NULL);
	ck_assert_int_eq(strlen(str), 0);

	rc = uinput_device_new_with_events(&uidev, "test device", &ids,
					   EV_ABS, ABS_X,
					   -1);
	ck_assert_msg(rc == 0, "Failed to create uinput device: %s", strerror(-rc));
	rc = libevdev_set_fd(dev, uinput_device_get_fd(uidev));
	ck_assert_msg(rc == 0, "Failed to init device: %s", strerror(-rc));;

	str = libevdev_get_name(dev);
	ck_assert_int_eq(strcmp(str, "test device"), 0);

	str = libevdev_get_phys(dev);
	ck_assert(str == NULL);

	str = libevdev_get_uniq(dev);
	ck_assert(str == NULL);

	ck_assert_int_eq(libevdev_get_bustype(dev), ids.bustype);
	ck_assert_int_eq(libevdev_get_vendor_id(dev), ids.vendor);
	ck_assert_int_eq(libevdev_get_product_id(dev), ids.product);
	ck_assert_int_eq(libevdev_get_version(dev), ids.version);
	ck_assert_int_eq(libevdev_get_driver_version(dev), EV_VERSION);

	uinput_device_free(uidev);
	libevdev_free(dev);
}
END_TEST

START_TEST(test_device_enable_bit)
{
	struct uinput_device* uidev;
	struct libevdev *dev, *dev2;
	struct input_absinfo abs;
	int rc;

	rc = test_create_device(&uidev, &dev,
				EV_ABS, ABS_X,
				-1);
	ck_assert_msg(rc == 0, "Failed to create device: %s", strerror(-rc));

	ck_assert(!libevdev_has_event_code(dev, EV_ABS, ABS_Y));
	ck_assert(!libevdev_has_event_type(dev, EV_REL));
	ck_assert(!libevdev_has_event_code(dev, EV_REL, REL_X));

	abs.minimum = 0;
	abs.maximum = 100;
	abs.fuzz = 1;
	abs.flat = 2;
	abs.resolution = 3;

	ck_assert_int_eq(libevdev_enable_event_code(dev, EV_ABS, ABS_Y, &abs), 0);
	ck_assert(libevdev_has_event_code(dev, EV_ABS, ABS_Y));

	ck_assert_int_eq(libevdev_enable_event_type(dev, EV_REL), 0);
	ck_assert(libevdev_has_event_type(dev, EV_REL));
	ck_assert(!libevdev_has_event_code(dev, EV_REL, REL_X));

	ck_assert_int_eq(libevdev_enable_event_code(dev, EV_REL, REL_X, NULL), 0);
	ck_assert(libevdev_has_event_code(dev, EV_REL, REL_X));

	/* make sure kernel device is unchanged */
	rc = libevdev_new_from_fd(uinput_device_get_fd(uidev), &dev2);
	ck_assert_msg(rc == 0, "Failed to init device: %s", strerror(-rc));
	ck_assert(libevdev_has_event_code(dev2, EV_ABS, ABS_X));
	ck_assert(!libevdev_has_event_code(dev2, EV_ABS, ABS_Y));
	ck_assert(!libevdev_has_event_type(dev2, EV_REL));
	ck_assert(!libevdev_has_event_code(dev2, EV_REL, REL_X));
	libevdev_free(dev2);

	uinput_device_free(uidev);
	libevdev_free(dev);
}
END_TEST

START_TEST(test_device_enable_bit_invalid)
{
	struct uinput_device* uidev;
	struct libevdev *dev;
	struct input_absinfo abs = {0};
	int rc;

	rc = test_create_device(&uidev, &dev,
				EV_ABS, ABS_X,
				-1);
	ck_assert_msg(rc == 0, "Failed to create device: %s", strerror(-rc));

	ck_assert_int_eq(libevdev_enable_event_code(dev, EV_ABS, ABS_MAX + 1, &abs), -1);
	ck_assert_int_eq(libevdev_enable_event_type(dev, EV_MAX + 1), -1);

	uinput_device_free(uidev);
	libevdev_free(dev);
}
END_TEST

START_TEST(test_device_disable_bit)
{
	struct uinput_device* uidev;
	struct libevdev *dev, *dev2;
	int rc;

	rc = test_create_device(&uidev, &dev,
				EV_ABS, ABS_X,
				EV_ABS, ABS_Y,
				EV_REL, REL_X,
				EV_REL, REL_Y,
				-1);
	ck_assert_msg(rc == 0, "Failed to create device: %s", strerror(-rc));

	ck_assert(libevdev_has_event_code(dev, EV_ABS, ABS_X));
	ck_assert(libevdev_has_event_code(dev, EV_ABS, ABS_Y));
	ck_assert(libevdev_has_event_type(dev, EV_REL));
	ck_assert(libevdev_has_event_code(dev, EV_REL, REL_X));
	ck_assert(libevdev_has_event_code(dev, EV_REL, REL_Y));

	ck_assert_int_eq(libevdev_disable_event_code(dev, EV_ABS, ABS_Y), 0);
	ck_assert(!libevdev_has_event_code(dev, EV_ABS, ABS_Y));

	ck_assert_int_eq(libevdev_disable_event_code(dev, EV_REL, REL_X), 0);
	ck_assert(!libevdev_has_event_code(dev, EV_REL, REL_X));
	ck_assert(libevdev_has_event_code(dev, EV_REL, REL_Y));
	ck_assert(libevdev_has_event_type(dev, EV_REL));

	ck_assert_int_eq(libevdev_disable_event_type(dev, EV_REL), 0);
	ck_assert(!libevdev_has_event_type(dev, EV_REL));
	ck_assert(!libevdev_has_event_code(dev, EV_REL, REL_X));
	ck_assert(!libevdev_has_event_code(dev, EV_REL, REL_Y));

	/* make sure kernel device is unchanged */
	rc = libevdev_new_from_fd(uinput_device_get_fd(uidev), &dev2);
	ck_assert_msg(rc == 0, "Failed to init device: %s", strerror(-rc));
	ck_assert(libevdev_has_event_code(dev2, EV_ABS, ABS_X));
	ck_assert(libevdev_has_event_code(dev2, EV_ABS, ABS_Y));
	ck_assert(libevdev_has_event_type(dev2, EV_REL));
	ck_assert(libevdev_has_event_code(dev2, EV_REL, REL_X));
	ck_assert(libevdev_has_event_code(dev2, EV_REL, REL_Y));
	libevdev_free(dev2);

	uinput_device_free(uidev);
	libevdev_free(dev);
}
END_TEST

START_TEST(test_device_disable_bit_invalid)
{
	struct uinput_device* uidev;
	struct libevdev *dev;
	int rc;

	rc = uinput_device_new_with_events(&uidev, "test device", DEFAULT_IDS,
					   EV_ABS, ABS_X,
					   -1);
	ck_assert_msg(rc == 0, "Failed to create uinput device: %s", strerror(-rc));
	rc = libevdev_new_from_fd(uinput_device_get_fd(uidev), &dev);
	ck_assert_msg(rc == 0, "Failed to init device: %s", strerror(-rc));


	ck_assert_int_eq(libevdev_disable_event_code(dev, EV_ABS, ABS_MAX + 1), -1);
	ck_assert_int_eq(libevdev_disable_event_type(dev, EV_MAX + 1), -1);

	uinput_device_free(uidev);
	libevdev_free(dev);
}
END_TEST

START_TEST(test_device_enable_kernel_bit)
{
	struct uinput_device* uidev;
	struct libevdev *dev, *dev2;
	struct input_absinfo abs;
	int rc;

	rc = test_create_device(&uidev, &dev,
				EV_ABS, ABS_X,
				-1);
	ck_assert_msg(rc == 0, "Failed to create device: %s", strerror(-rc));

	ck_assert(!libevdev_has_event_code(dev, EV_ABS, ABS_Y));
	ck_assert(!libevdev_has_event_type(dev, EV_REL));
	ck_assert(!libevdev_has_event_code(dev, EV_REL, REL_X));

	abs.value = 0;
	abs.minimum = 0;
	abs.maximum = 100;
	abs.fuzz = 1;
	abs.flat = 2;
	abs.resolution = 3;

	ck_assert_int_eq(libevdev_kernel_enable_event_code(dev, EV_ABS, ABS_Y, &abs), 0);
	ck_assert(libevdev_has_event_code(dev, EV_ABS, ABS_Y));

	ck_assert_msg(libevdev_kernel_enable_event_type(dev, EV_REL), 0);
	ck_assert(libevdev_has_event_type(dev, EV_REL));
	ck_assert(!libevdev_has_event_code(dev, EV_REL, REL_X));

	ck_assert_int_eq(libevdev_kernel_enable_event_code(dev, EV_REL, REL_X, NULL), 0);
	ck_assert(libevdev_has_event_code(dev, EV_REL, REL_X));

	/* make sure kernel device is unchanged */
	rc = libevdev_new_from_fd(uinput_device_get_fd(uidev), &dev2);
	ck_assert_msg(rc == 0, "Failed to init device: %s", strerror(-rc));
	ck_assert(libevdev_has_event_code(dev2, EV_ABS, ABS_X));
	ck_assert(libevdev_has_event_code(dev2, EV_ABS, ABS_Y));
	ck_assert(libevdev_has_event_type(dev2, EV_REL));
	ck_assert(libevdev_has_event_code(dev2, EV_REL, REL_X));
	libevdev_free(dev2);

	uinput_device_free(uidev);
	libevdev_free(dev);
}
END_TEST

Suite *
libevdev_has_event_test(void)
{
	Suite *s = suite_create("libevdev_has_event tests");

	TCase *tc = tcase_create("event type");
	tcase_add_test(tc, test_ev_bit_limits);
	tcase_add_test(tc, test_has_ev_bit);
	suite_add_tcase(s, tc);

	tc = tcase_create("event codes");
	tcase_add_test(tc, test_event_codes);
	tcase_add_test(tc, test_event_code_limits);
	suite_add_tcase(s, tc);

	tc = tcase_create("ev_rep");
	tcase_add_test(tc, test_ev_rep);
	tcase_add_test(tc, test_ev_rep_values);
	suite_add_tcase(s, tc);

	tc = tcase_create("input properties");
	tcase_add_test(tc, test_input_props);
	suite_add_tcase(s, tc);

	tc = tcase_create("multitouch info");
	tcase_add_test(tc, test_no_slots);
	tcase_add_test(tc, test_slot_number);
	suite_add_tcase(s, tc);

	tc = tcase_create("device info");
	tcase_add_test(tc, test_device_name);
	suite_add_tcase(s, tc);

	tc = tcase_create("device bit manipulation");
	tcase_add_test(tc, test_device_enable_bit);
	tcase_add_test(tc, test_device_enable_bit_invalid);
	tcase_add_test(tc, test_device_disable_bit);
	tcase_add_test(tc, test_device_disable_bit_invalid);
	tcase_add_test(tc, test_device_enable_kernel_bit);
	suite_add_tcase(s, tc);

	return s;
}

