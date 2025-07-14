import { defineConfig } from 'vitest/config';

export default defineConfig({
  test: {
    name: 'lynx-types',
    include: [],
    typecheck: {
      include: ['test/**/*.test-d.ts'],
    },
  },
});
