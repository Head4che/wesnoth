/*
   Copyright (C) 2009 by Chris Hopman <cjhopman@gmail.com>
   Part of the Battle for Wesnoth Project http://www.wesnoth.org/

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2
   or at your option any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

#ifndef SHARED_OBJECT_HPP_INCLUDED
#define SHARED_OBJECT_HPP_INCLUDED

#include <climits>
#include <cassert>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>

template <typename T>
struct shared_node {
	T val;
	unsigned long count;
	shared_node() : val(), count(0) { }
	shared_node(const T& o) : val(o), count(0) { }
	static const unsigned long max_count = ULONG_MAX;
};

struct increment_count {
	template <typename T>
	void operator()(T& o) {
		++o.count;
	}
};

struct decrement_count {
	template <typename T>
	void operator()(T& o) {
		--o.count;
	}
};

template <typename T>
struct hasher {
	size_t operator()(const T& o) {
		return hash_value(o.val);
	}
};

template <typename T, typename node = shared_node<T> >
class shared_object {
public:
	typedef T type;

	shared_object() : val_(0) { set(T()); }

	template <typename U>
	shared_object(const U& o) : val_(0) { set(o); }

	shared_object(const shared_object& o) : val_(o.val_) {
		assert(valid());
		index().modify(index().iterator_to(*val_), increment_count());
	}

	operator T() const {
		assert(valid());
		return val_->val;
	}

	shared_object& operator=(const shared_object& o) {
		if (val_ == o.val_) return *this;
		shared_object tmp(o);
		swap(tmp);
		return *this;
	}

	~shared_object() { clear(); }

	void set(const T& o) {
		if (val_ && o == get()) return;
		clear();

		val_ = &*index().insert(node(o)).first;
		index().modify(index().iterator_to(*val_), increment_count());

		assert((val_->count) < (node::max_count));
	}

	const T& get() const {
		assert(valid());
		return val_->val;
	}

	void swap(shared_object& o) {
		std::swap(val_, o.val_);
	}

	bool valid() const {
		return val_ != NULL;
	}

	const node* ptr() const {
		return val_;
	}

protected:

	typedef boost::multi_index_container<
		node,
		boost::multi_index::indexed_by<
			boost::multi_index::hashed_unique<
				BOOST_MULTI_INDEX_MEMBER(node, T, val)
			>
		>
	> hash_map;

	typedef typename hash_map::template nth_index<0>::type hash_index;

	static hash_map& map() { static hash_map* map = new hash_map; return *map; }
	static hash_index& index() { return map().template get<0>(); }

	const node* val_;

	void clear() {
		if (!val_) return;
		index().modify(index().iterator_to(*val_), decrement_count());

		if (val_->count == 0) index().erase(index().iterator_to(*val_));
		val_ = 0;
	}

};

template <typename T>
bool operator==(const shared_object<T>& a, const shared_object<T>& b) {
	return a.ptr() == b.ptr() ? true : a.get() == b.get();
}

template <typename T>
bool operator<(const shared_object<T>& a, const shared_object<T>& b) {
	assert(a.valid());
	assert(b.valid());
	return a.get() < b.get();
}

template <typename T>
std::ostream& operator<<(std::ostream& stream, const shared_object<T>& o) {
	assert(o.valid());
	return stream << o.get();
}

template <typename T>
std::istream& operator>>(std::istream& stream, shared_object<T>& o) {
	T t;
	std::istream& ret = stream >> t;
	o.set(t);
	return ret;
}

#endif
