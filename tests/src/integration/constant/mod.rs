use ext_php_rs::builders::ClassBuilder;
use ext_php_rs::flags::ConstantFlags;
use ext_php_rs::prelude::*;
use std::sync::LazyLock;

static STATIC_DYN_VEC: LazyLock<Vec<i64>> = LazyLock::new(|| vec![7, 8, 9]);

fn modify_class(builder: ClassBuilder) -> ClassBuilder {
    let dyn_vec: &'static Vec<i64> = &STATIC_DYN_VEC;
    builder
        .constant_lazy(
            "CLASS_VEC_MANUAL",
            vec!["hello".to_string(), "world".to_string()],
            &[],
            ConstantFlags::Public,
        )
        .expect("Failed to add manual constant")
        .dyn_constant("CLASS_VEC_DYN", dyn_vec, &[], ConstantFlags::Public)
        .expect("Failed to add dyn constant")
}

#[php_class]
#[php(modifier = modify_class)]
pub struct TestConstantVisibility;

#[php_impl]
impl TestConstantVisibility {
    const PUBLIC_CONST: i64 = 1;
    #[php(vis = "public")]
    const EXPLICIT_PUBLIC_CONST: i64 = 2;
    #[php(vis = "protected")]
    const PROTECTED_CONST: &'static str = "protected";
    #[php(vis = "private")]
    const PRIVATE_CONST: bool = true;
    #[php(vis = "public")]
    const PRIVATE_ARR: [i64; 4] = [1, 3, 5, 5];

    pub fn read_private() -> bool {
        Self::PRIVATE_CONST
    }
}

pub fn build_module(builder: ModuleBuilder) -> ModuleBuilder {
    builder
        .class::<TestConstantVisibility>()
        .constant(("TEST_GLOBAL_ARRAY", [10i64, 20i64, 30i64], &[]))
        .constant(("TEST_GLOBAL_VEC", vec![100i64, 200i64, 300i64], &[]))
}

#[cfg(test)]
mod tests {
    #[test]
    fn constant_visibility_works() {
        assert!(crate::integration::test::run_php("constant/constant.php"));
    }
}
