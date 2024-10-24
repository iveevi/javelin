#pragma once

// Representing values which may change over frames
// TODO: critical with timer reset...
template <typename T>
class Critical {
	T item;
	T next;
	mutable bool up_to_date = false;
public:
	Critical(const T &value, bool up_to_date_ = false)
		: next(value), up_to_date(up_to_date_) {
		if (up_to_date)
			item = next;
	}

	void queue(const T &value) {
		next = value;
		up_to_date = false;
	}

	const T &current() {
		return item;
	}

	const T &updated() {
		if (old()) {
			item = next;
			up_to_date = true;
		}

		return item;
	}

	const T *operator->() {
		return &item;
	}	

	bool old() const {
		return !up_to_date;
	}
};