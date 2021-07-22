//
// Created by shawnfeng on 7/11/21.
// Copyright (c) 2021 shawnfeng. All rights reserved.
//
#pragma once

#include <exception>

namespace disruptor {

class Eof : public std::exception {
 public:
  const char* what() const noexcept override { return "eof"; }
};

}  // namespace disruptor
