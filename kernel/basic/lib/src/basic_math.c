#include <basic_math.h>

#ifdef __aeabi_idiv

inline uint32_t div_u32(uint32_t numerator, uint32_t denominator) {
	return numerator / denominator;
}

inline int32_t div_i32(int32_t numerator, int32_t denominator) {
	return numerator / denominator;
}

inline uint32_t mod_u32(uint32_t numerator, uint32_t denominator) {
	return numerator % denominator;
}

inline int32_t mod_i32(int32_t numerator, int32_t denominator) {
	return numerator % denominator;
}

#else

struct qr {
	uint32_t q;		/* computed quotient */
	uint32_t r;		/* computed remainder */
	uint32_t q_n;		/* specficies if quotient shall be negative */
	uint32_t r_n;		/* specficies if remainder shall be negative */
};

static void division_qr(uint32_t n, uint32_t p, struct qr *qr) {
	uint32_t i = 1, q = 0;
	if (p == 0) {
		qr->r = 0xFFFFFFFF;	/* division by 0 */
		return;
	}

	while ((p >> 31) == 0) {
		i = i << 1;	/* count the max division steps */
		p = p << 1;     /* increase p until it has maximum size*/
	}

	while (i > 0) {
		q = q << 1;	/* write bit in q at index (size-1) */
		if (n >= p)
		{
			n -= p;
			q++;
		}
		p = p >> 1; 	/* decrease p */
		i = i >> 1; 	/* decrease remaining size in q */
	}
	qr->r = n;
	qr->q = q;
}

static void uint_div_qr(uint32_t numerator, uint32_t denominator, struct qr *qr) {
	division_qr(numerator, denominator, qr);

	/* negate quotient and/or remainder according to requester */
	if (qr->q_n)
		qr->q = -qr->q;
	if (qr->r_n)
		qr->r = -qr->r;
}

//uint32_t __aeabi_uidiv(uint32_t numerator, uint32_t denominator) {
uint32_t div_u32(uint32_t numerator, uint32_t denominator) {
	struct qr qr = { .q_n = 0, .r_n = 0 };
	uint_div_qr(numerator, denominator, &qr);
	return qr.q;
}

unsigned __aeabi_uidiv(unsigned numerator, unsigned denominator) {
	return div_u32(numerator, denominator);
}

unsigned __ret_uidivmod_values(unsigned numerator, unsigned denominator);
unsigned __aeabi_uidivmod(unsigned numerator, unsigned denominator) {
	struct qr qr = { .q_n = 0, .r_n = 0 };
	uint_div_qr(numerator, denominator, &qr);
	return __ret_uidivmod_values(qr.q, qr.r);
}

//uint32_t __aeabi_uidivmod(uint32_t numerator, uint32_t denominator) {
uint32_t mod_u32(uint32_t numerator, uint32_t denominator) {
	struct qr qr = { .q_n = 0, .r_n = 0 };
	uint_div_qr(numerator, denominator, &qr);
	return qr.r;
}


//int32_t __aeabi_idiv(int32_t numerator, int32_t denominator) {
int32_t div_i32(int32_t numerator, int32_t denominator) {
	struct qr qr = { .q_n = 0, .r_n = 0 };

	if (((numerator < 0) && (denominator > 0)) ||
	    ((numerator > 0) && (denominator < 0)))
		qr.q_n = 1;	/* quotient shall be negate */
	if (numerator < 0) {
		numerator = -numerator;
		qr.r_n = 1;	/* remainder shall be negate */
	}
	if (denominator < 0)
		denominator = -denominator;

	uint_div_qr(numerator, denominator, &qr);
	return qr.q;
}

signed __aeabi_idiv(signed numerator, signed denominator) {
	return div_i32(numerator, denominator);
}

//int32_t __aeabi_idivmod(int32_t numerator, int32_t denominator) {
int32_t mod_i32(int32_t numerator, int32_t denominator) {
	struct qr qr = { .q_n = 0, .r_n = 0 };

	if (((numerator < 0) && (denominator > 0)) ||
	    ((numerator > 0) && (denominator < 0)))
		qr.q_n = 1;	/* quotient shall be negate */
	if (numerator < 0) {
		numerator = -numerator;
		qr.r_n = 1;	/* remainder shall be negate */
	}
	if (denominator < 0)
		denominator = -denominator;

	uint_div_qr(numerator, denominator, &qr);
	return qr.r;
}

signed __ret_idivmod_values(signed numerator, signed denominator);
signed __aeabi_idivmod(signed numerator, signed denominator) {
	struct qr qr = { .q_n = 0, .r_n = 0 };

	if (((numerator < 0) && (denominator > 0)) ||
	    ((numerator > 0) && (denominator < 0)))
		qr.q_n = 1;	/* quotient shall be negate */
	if (numerator < 0) {
		numerator = -numerator;
		qr.r_n = 1;	/* remainder shall be negate */
	}
	if (denominator < 0)
		denominator = -denominator;

	uint_div_qr(numerator, denominator, &qr);
	return __ret_idivmod_values(qr.q, qr.r);
}

#endif