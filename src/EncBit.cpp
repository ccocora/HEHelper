#include "EncBit.h"

EncBit::EncBit(FHEPubKey& key, EncryptedArray& ea)
: m_key(key), m_ea(ea), data(key) {}

EncBit::EncBit(FHEPubKey& key, EncryptedArray& ea, bool data)
: m_key(key), m_ea(ea), data(key) {
	this->encrypt(data);
}

void EncBit::encrypt(bool data) {
	PlaintextArray pa(this->m_ea);
	pa.encode(data);
	this->m_ea.encrypt(this->data, this->m_key, pa);
}

bool EncBit::decrypt(const FHESecKey& key) {
	vector<long> aux;
	this->m_ea.decrypt(this->data, key, aux);
	bool result = aux[0];
	return result;
}

EncBit& EncBit::operator=(const EncBit& rhs) {
	// this->m_key = rhs.m_key;
	// this->m_ea = rhs.m_ea;
	this->data = rhs.data;
	return *this;
}

EncBit EncBit::operator!() const {
	EncBit one(this->m_key, this->m_ea, true);

	EncBit result = *this;
	result.data.addCtxt(one.data);
	return result;
}

EncBit EncBit::operator^(const EncBit &rhs) const {
	EncBit result = *this;
	result.data.addCtxt(rhs.data);
	return result;
}

EncBit EncBit::operator&(const EncBit &rhs) const {
	EncBit result = *this;
	result.data.multiplyBy(rhs.data);
	return result;
}

EncBit EncBit::operator|(const EncBit &rhs) const {
	return !(!(*this) & !rhs);
}

EncBit EncBit::operator==(const EncBit& rhs) const {
	return !(*this ^ rhs);
}

EncBit EncBit::operator&&(const EncBit& rhs) const {
	return *this & rhs;
}

EncBit EncBit::operator||(const EncBit& rhs) const {
	return *this | rhs;
}

void EncBit::cas(const EncBit& condition, const EncBit& rhs) {
	*this = (condition && rhs) || (!condition && *this);
}
