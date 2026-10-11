// by Marius Versteegen, 2026
// HTTP constants shared by server_v7 and client_v7.

#pragma once

namespace crt
{
	// client_v7 (the test app) sends this header with every request, so it may fetch the
	// login-protected /grid page without logging in.
	static constexpr const char* TEST_CLIENT_HEADER = "X-Sensorgrid-Test";
	static constexpr const char* TEST_CLIENT_TOKEN = "sg7-client-test";
}
