const freq_in = 275000000;
const divider_one_max = 65536;
const divider_two_max = 65536;
const target_freq = 22058;
let best_error = Infinity;
let best_divider_one = 0;
let best_divider_two = 0;

for (let divider_one = 2; divider_one < divider_one_max; divider_one++) {
  for (let divider_two = 2; divider_two < divider_two_max; divider_two++) {
    const freq_out = freq_in / divider_one / divider_two;
    const error = Math.abs(freq_out - target_freq);

    if (error < best_error) {
        best_error = error;
        best_divider_one = divider_one;
        best_divider_two = divider_two;
    }  
  }
}

console.log(`Best divider one: ${best_divider_one}`);
console.log(`Best divider two: ${best_divider_two}`);
console.log(`Best error: ${best_error}`);
