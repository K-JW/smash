/**
 * @file
 *
 * This is the main include file for Einhard.
 *
 * Copyright 2010 Matthias Bach <marix@marix.org>
 * Copyright 2014 Matthias Kretz <kretz@kde.org>
 *
 * This file is part of Einhard.
 * 
 * Einhard is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * Einhard is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with Einhard.  If not, see <http://www.gnu.org/licenses/>.
 */

/** \mainpage Einhard Logging Library
 *
 * \section intro_sec Introduction
 *
 * Einhard aims at being a lightweight logging library with the following features:
 *  - Severity filtering
 *  - Automatic output colorization
 *  - Output timestamping
 *  - Low performance overhead
 *
 * To use Einhard all you need to do is create an einhard::Logger object and push messages into its output
 * streams.
 *
 * \code
 * using namespace einhard;
 * Logger logger( INFO );
 * logger.trace() << "Trace message"; // will not be printed
 * logger.error() << "Error message"; // will be printed
 * \endcode
 *
 * To reduce the performance overhad you can specify NDEBUG during compile. This will disable trace
 * and debug messages in a way that should allow the compilers dead code elimination to remove
 * everything that is only pushed into such a stream.
 *
 * \section install_sec Installation
 *
 * Einhard is build using cmake. You can install it using the usual cmake triplet:
 * \code
 * cmake
 * make
 * sudo make install
 * \endcode
 * If you want to build a static lib or install to a custom path you can use the usual cmake
 * utilities to adjust the configuration.
 */

#include <iostream>
#include <iomanip>
#include <ctime>
#include <sstream>
#include <bitset>

// This C header is sadly required to check whether writing to a terminal or a file
#include <cstdio>

#include <unistd.h>  // for isatty

#ifdef __GNUC__
#define EINHARD_ALWAYS_INLINE_ __attribute__((always_inline))
#else
#define EINHARD_ALWAYS_INLINE_
#endif

/**
 * This namespace contains all objects required for logging using Einhard.
 */
namespace einhard
{
	/**
	 * Version string of the Einhard library
	 */
	extern char const VERSION[];

	/**
	 * Specification of the message severity.
	 *
	 * In output each level automatically includes the higher levels.
	 */
	enum LogLevel
	{
		ALL,   /**< Log all message */
		TRACE, /**< The lowes severity for messages describing the program flow */
		DEBUG, /**< Debug messages */
		INFO,  /**< Messages of informational nature, expected processing time e.g. */
		WARN,  /**< Warning messages */
		ERROR, /**< Non-fatal errors */
		FATAL, /**< Messages that indicate terminal application failure */
		OFF    /**< If selected no messages will be output */
	};

	/**
	 * Retrieve a human readable representation of the given log level value.
	 */
	template <LogLevel> const char *getLogLevelString() noexcept;
	const char *getLogLevelString( LogLevel level );
	template <LogLevel> const char *colorForLogLevel() noexcept;

	/**
	 * A stream modifier that allows to colorize the log output.
	 */
	template <typename Parent> class Color
	{
	public:
		/// The default color modifier only affects the next object in the stream.
		EINHARD_ALWAYS_INLINE_ Color() noexcept : reset( true )
		{
		}
		/// With the ~ operator the color modifier affects the rest of the stream (or until
		/// another color object is used).
		EINHARD_ALWAYS_INLINE_ Color<Parent> operator~() const noexcept
		{
			return {false};
		}
		EINHARD_ALWAYS_INLINE_ char const *ansiCode() const noexcept
		{
			return Parent::ANSI();
		}
		EINHARD_ALWAYS_INLINE_ bool resetColor() const noexcept
		{
			return reset;
		}

	private:
		EINHARD_ALWAYS_INLINE_ Color( bool r ) noexcept : reset( r )
		{
		}
		bool reset;
	};
#define _COLOR( name, code )                                                                                           \
	struct name##_t_                                                                                               \
	{                                                                                                              \
		static char const *ANSI() noexcept                                                                     \
		{                                                                                                      \
			return "\33[" code "m";                                                                        \
		}                                                                                                      \
	};                                                                                                             \
	typedef Color<name##_t_> name

	_COLOR(DGray,   "01;30");
	_COLOR(Black,   "00;30");
	_COLOR(Red,     "01;31");
	_COLOR(DRed,    "00;31");
	_COLOR(Green,   "01;32");
	_COLOR(DGreen,  "00;32");
	_COLOR(Yellow,  "01;33");
	_COLOR(Orange,  "00;33");
	_COLOR(Blue,    "01;34");
	_COLOR(DBlue,   "00;34");
	_COLOR(Magenta, "01;35");
	_COLOR(DMagenta,"00;35");
	_COLOR(Cyan,    "01;36");
	_COLOR(DCyan,   "00;36");
	_COLOR(White,   "01;37");
	_COLOR(Gray,    "00;37");
	_COLOR(NoColor, "0"    );
#undef _COLOR

	/**
	 * A minimal class that implements the output stream operator to do nothing. This completely
	 * eliminates the output stream statements from the resulting binary.
	 */
	struct DummyOutputFormatter
	{
		template <typename T>
#ifdef __GNUC__
			__attribute__((__always_inline__))
#endif
			DummyOutputFormatter &operator<<( const T & ) noexcept
		{
			return *this;
		}
	};

	// The output stream to print to
	extern thread_local std::ostringstream t_out;

	/**
	 * A wrapper for the output stream taking care proper formatting and colorization of the output.
	 */
	class OutputFormatter
	{
		private:
			// Pointer to the thread_local stringstream (if enabled)
			std::ostringstream *out;
			// Whether output is enabled
			const bool enabled;
			// Whether to colorize the output
			const bool colorize;
			// Whether the color needs to be reset with the next operator<<
			bool resetColor = false;

		public:
			OutputFormatter( const OutputFormatter & ) = delete;
			OutputFormatter( OutputFormatter && ) = default;

			template <LogLevel VERBOSITY>
			EINHARD_ALWAYS_INLINE_ OutputFormatter( bool enabled_, bool const colorize_,
								const char *areaName,
								std::integral_constant<LogLevel, VERBOSITY> )
			    : enabled( enabled_ ), colorize( colorize_ )
			{
				if( enabled )
				{
					doInit<VERBOSITY>( areaName );
				}
			}

			template <typename T> OutputFormatter &operator<<( const Color<T> &col )
			{
				if( enabled && colorize )
				{
					*out << col.ansiCode();
					resetColor = col.resetColor();
				}
				return *this;
			}

			template <typename T> EINHARD_ALWAYS_INLINE_ OutputFormatter &operator<<( const T &msg )
			{
				if( enabled )
				{
					*out << msg;
					checkColorReset();
				}
				return *this;
			}

			EINHARD_ALWAYS_INLINE_
			~OutputFormatter( )
			{
				if( enabled )
				{
					doCleanup();
				}
			}

		private:
			template <LogLevel VERBOSITY> void doInit( const char *areaName );

			void checkColorReset();

			void doCleanup() noexcept;
	};

	/**
     * A Logger object can be used to output messages to stdout.
     *
     * The Logger object is created with a certain verbosity. All messages of a more verbose
     * LogLevel will be filtered out. The way the class is build this can happen at compile
     * time up to the level restriction given by the template parameter.
     *
     * The class can automatically detect non-tty output and will not colorize output in that case.
     */ 
	template<LogLevel MAX = ALL> class Logger
	{
		private:
			const char *areaName = nullptr;
			LogLevel verbosity;
			bool colorize;

		public:
			/**
			 * Create a new Logger object.
			 *
			 * The object will automatically colorize output on ttys and not colorize output
			 * on non ttys.
			 */
			Logger( const LogLevel verbosity = WARN ) : verbosity( verbosity )
			{
				// use some, sadly not c++-ways to figure out whether we are writing ot a terminal
				// only colorize when we are writing ot a terminal
				colorize = isatty( fileno( stdout ) );
			};
			/**
			 * Create a new Logger object explicitly selecting whether to colorize the output or not.
			 *
			 * Be aware that if output colorization is selected output will even be colorized if
			 * output is to a non tty.
			 */
			Logger( const LogLevel verbosity, const bool colorize )
			    : verbosity( verbosity ), colorize( colorize ) {};

			/**
			 * Set an area name. This will be printed after the LogLevel to identify the
			 * place in the code where the output is coming from. This can be used to
			 * identify the different Logger objects in the log output.
			 *
			 * \param name A pointer to a constant string. The pointer must stay valid
			 *             for as long as the Logger object is used. You may set this to nullptr to
			 *             unset the name.
			 */
			void setAreaName( const char *name )
			{
				areaName = name;
			}

			/** Access to the trace message stream. */
#ifdef NDEBUG
			DummyOutputFormatter trace() const noexcept
			{
				return DummyOutputFormatter();
			}
#else
			OutputFormatter trace() const
			{
				return {isEnabled<TRACE>(), colorize, areaName,
					std::integral_constant<LogLevel, TRACE>()};
			}
#endif
			/** Access to the debug message stream. */
#ifdef NDEBUG
			DummyOutputFormatter debug() const noexcept
			{
				return DummyOutputFormatter();
			}
#else
			OutputFormatter debug() const
			{
				return {isEnabled<DEBUG>(), colorize, areaName,
					std::integral_constant<LogLevel, DEBUG>()};
			}
#endif
			/** Access to the info message stream. */
			OutputFormatter info() const
			{
				return {isEnabled<INFO>(), colorize, areaName,
					std::integral_constant<LogLevel, INFO>()};
			}
			/** Access to the warning message stream. */
			OutputFormatter warn() const
			{
				return {isEnabled<WARN>(), colorize, areaName,
					std::integral_constant<LogLevel, WARN>()};
			}
			/** Access to the error message stream. */
			OutputFormatter error() const
			{
				return {isEnabled<ERROR>(), colorize, areaName,
					std::integral_constant<LogLevel, ERROR>()};
			}
			/** Access to the fatal message stream. */
			OutputFormatter fatal() const
			{
				return {isEnabled<FATAL>(), colorize, areaName,
					std::integral_constant<LogLevel, FATAL>()};
			}

			template <LogLevel LEVEL> bool isEnabled() const noexcept
			{
#ifdef NDEBUG
				if( LEVEL == DEBUG || LEVEL == TRACE ) {
					return false;
				}
#endif
				return ( MAX <= LEVEL && verbosity <= LEVEL );
			}

			/** Modify the verbosity of the Logger.
 			 *
			 * Be aware that the verbosity can not be increased over the level given by the template
			 * parameter
			 */
			inline void setVerbosity( LogLevel verbosity ) noexcept
			{
				this->verbosity = verbosity;
			}
			/** Retrieve the current log level.
			 *
			 * If you want to check whether messages of a certain LogLevel will be output the
			 * method isEnabled() should be
			 * preffered.
			 */
			inline LogLevel getVerbosity() const noexcept
			{
				return this->verbosity;
			}
			/**
			 * Retrieve a human readable representation of the current log level
			 */
			inline char const * getVerbosityString( ) const
			{
				return getLogLevelString(verbosity);
			}
			/**
			 * Select whether the output stream should be colorized.
			 */
			void setColorize( bool colorize ) noexcept
			{
				this->colorize = colorize;
			}
			/**
			 * Check whether the output stream is colorized.
			 */
			void getColorize() const noexcept
			{
				return this->colorize;
			}
	};
}

// vim: ts=4 sw=4 tw=100 noet
