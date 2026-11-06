<?php

declare(strict_types=1);

$reflection = new ReflectionClass(TestConstantVisibility::class);
assert($reflection->getReflectionConstant('PUBLIC_CONST')->isPublic());
assert($reflection->getReflectionConstant('EXPLICIT_PUBLIC_CONST')->isPublic());
assert($reflection->getReflectionConstant('PROTECTED_CONST')->isProtected());
assert($reflection->getReflectionConstant('PRIVATE_CONST')->isPrivate());

assert(TestConstantVisibility::PUBLIC_CONST === 1);
assert(TEST_GLOBAL_ARRAY === [10, 20, 30]);
assert(TEST_GLOBAL_VEC === [100, 200, 300]);
assert(TestConstantVisibility::PRIVATE_ARR === [1, 3, 5, 5]);
assert(TestConstantVisibility::CLASS_VEC_MANUAL === ['hello', 'world']);
assert(TestConstantVisibility::CLASS_VEC_DYN === [7, 8, 9]);
assert(TestConstantVisibility::EXPLICIT_PUBLIC_CONST === 2);
assert(TestConstantVisibility::readPrivate() === true);
assert($reflection->getConstant('PROTECTED_CONST') === 'protected');

try {
    TestConstantVisibility::PRIVATE_CONST;
    assert(false, 'a private constant must not be readable from outside the class');
} catch (Error $e) {
    assert(str_contains($e->getMessage(), 'Cannot access private constant'), $e->getMessage());
}

try {
    TestConstantVisibility::PROTECTED_CONST;
    assert(false, 'a protected constant must not be readable from outside the class');
} catch (Error $e) {
    assert(str_contains($e->getMessage(), 'Cannot access protected constant'), $e->getMessage());
}

final class ConstantChild extends TestConstantVisibility
{
    public static function protectedValue(): string
    {
        return self::PROTECTED_CONST;
    }
}

assert(ConstantChild::protectedValue() === 'protected');

// Verify memory stability (no leaks on repeated access or COW)
$start = memory_get_usage();
for ($i = 0; $i < 1000; $i++) {
    $a = TEST_GLOBAL_ARRAY;
    $v = TEST_GLOBAL_VEC;
    $c = TestConstantVisibility::PRIVATE_ARR;
    $m = TestConstantVisibility::CLASS_VEC_MANUAL;
    $d = TestConstantVisibility::CLASS_VEC_DYN;
    // Copy-on-write mutation
    $a[] = $i;
    unset($a, $v, $c, $m, $d);
}
$diff = memory_get_usage() - $start;
assert($diff === 0, "Memory leak detected in constant array access: {$diff} bytes");
assert(TEST_GLOBAL_ARRAY === [10, 20, 30], "Constant must remain immutable");

