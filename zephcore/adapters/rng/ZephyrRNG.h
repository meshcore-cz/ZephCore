/*
 * SPDX-License-Identifier: Apache-2.0
 * Zephyr CSPRNG implementation
 */

#pragma once

#include <mesh/RNG.h>
#include <stddef.h>
#include <string.h>

namespace mesh {

class ZephyrRNG : public RNG {
public:
	void random(uint8_t *dest, size_t sz) override;

	/* Layered entropy mixer for one-time first-boot identity-key
	 * generation. Combines:
	 *   1. sys_csrand_get (early)        — CSPRNG, strong on nRF/MG24
	 *   2. HWINFO unique device ID       — per-device uniqueness
	 *   3. Optional caller-supplied data — e.g. ADC LSB noise samples
	 *   4. CPU cycle-counter jitter      — main entropy source
	 *                                       (NIST SP 800-90B class)
	 *   5. sys_csrand_get (late)         — catches mid-boot radio init
	 *   6. CPU cycle-counter jitter #2   — independent timing window
	 * Conditioned via SHA-512. NIST-style health checks on jitter
	 * samples; reboots on degenerate output.
	 * Blocks for ~250-300ms — only called at first-boot identity gen.
	 *
	 * Output is suitable as an Ed25519 seed regardless of platform
	 * TRNG state at boot. */
	static void mixIdentitySeed(uint8_t *out, size_t out_len,
				    const uint8_t *extra = nullptr,
				    size_t extra_len = 0);
};

/* Thin wrapper that returns pre-supplied bytes from the mesh::RNG
 * interface. Used to feed a mixed seed into mesh::LocalIdentity(RNG*)
 * without changing the LocalIdentity API. One-shot — bytes are
 * consumed sequentially across calls. */
class SeededRNG : public RNG {
public:
	SeededRNG(const uint8_t *seed_data, size_t len)
		: _data(seed_data), _avail(len) {}

	void random(uint8_t *dest, size_t sz) override {
		size_t n = (sz <= _avail) ? sz : _avail;
		if (n > 0) memcpy(dest, _data, n);
		_data += n;
		_avail -= n;
		if (n < sz) memset(dest + n, 0, sz - n);
	}

private:
	const uint8_t *_data;
	size_t _avail;
};

} /* namespace mesh */
