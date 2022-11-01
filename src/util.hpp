#pragma once

#include <stdexcept>

class CompileException : public std::exception {
  public:
	const int position;
	const std::string error;

	explicit CompileException(int position, const std::string &error)
	    : position(position), error(error),
	      what_msg("At position " + std::to_string(position) + ": " + error) {}

	virtual const char *what() const noexcept {
		return what_msg.c_str();
	}

  private:
	const std::string what_msg;
};
