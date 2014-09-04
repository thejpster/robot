"""
Wrapper around the main Scons file (./src/SConscript) which puts all the
intermediates in ./bin

Copyright (c) 2014 theJPster (pwrs@hejpster.org.uk)

PWRS is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

PWRS is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with PWRS.  If not, see <http://www.gnu.org/licenses/>.
"""

# Use ./bin for building to keep the source tree clean
VariantDir('bin', 'src', duplicate=0)
# We say ./bin/X but we mean ./src/X, thanks to the line above
SConscript('bin/SConscript')
