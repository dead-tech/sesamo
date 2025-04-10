#ifndef SESAMO_COMMON_HPP
#define SESAMO_COMMON_HPP

#define TRY(expression) ({ \
    auto result = (expression); \
    if (!result) return std::unexpected { result.error() }; \
    *result; })

#endif // SESAMO_COMMON_HPP
