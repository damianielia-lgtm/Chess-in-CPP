#pragma once

#include <stdexcept>

struct AppError : std::runtime_error { using std::runtime_error::runtime_error; };

struct UserError : AppError { using AppError::AppError; };
struct CommandError : UserError { using UserError::UserError; };
struct FenError : UserError { using UserError::UserError; };
struct IllegalMoveError : UserError { using UserError::UserError; };
struct StorageError : UserError { using UserError::UserError; };
struct SessionError : UserError { using UserError::UserError; };
struct PgnError : UserError { using UserError::UserError; };
struct GameError : UserError { using UserError::UserError; };

struct OperationalError : AppError { using AppError::AppError; };
struct StorageIoError : OperationalError { using OperationalError::OperationalError; };
struct StockfishError : OperationalError { using OperationalError::OperationalError; };
