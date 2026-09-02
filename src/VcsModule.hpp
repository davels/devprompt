/**
 * @file VcsModule.hpp
 */

#pragma once

#include <string>

namespace devprompt
{ struct Options; }

namespace devprompt::VcsModule
{

bool load(const Options& options);

void remote_icon(std::string& out);
void branch(std::string& out);
void state(std::string& out);
void wt_icon(std::string& out);
void status(std::string& out);
void divergence(std::string& out);




//std::string format(const Options& options);
}

