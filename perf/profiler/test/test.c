/* test.c ---
 *
 * Filename: test.c
 * Description:
 * Author: yuan
 * Maintainer:
 * Created: ons jul  3 10:17:44 2024 (+0200)
 * Version:
 * Package-Requires: ()
 * Last-Updated:
 *           By:
 *     Update #: 0
 * URL:
 * Doc URL:
 * Keywords:
 * Compatibility:
 *
 */

/* Commentary:
 *
 *
 *
 */

/* Change Log:
 *
 *
 */

/* This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Emacs.  If not, see <https://www.gnu.org/licenses/>.
 */

/* Code: */

#include <stdio.h>
#include <unistd.h>

int main() {
  while (1) {
      sleep(1);
      printf("wake \n");
  }
  return 0;
}

/* test.c ends here */
