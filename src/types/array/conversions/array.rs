use super::super::ZendHashTable;
use crate::{
    boxed::ZBox,
    convert::{FromZval, IntoZval},
    error::{Error, Result},
    flags::DataType,
    types::Zval,
};

use std::convert::TryFrom;
use std::mem::MaybeUninit;

impl<'a, T, const N: usize> TryFrom<&'a ZendHashTable> for [T; N]
where
    T: FromZval<'a>,
{
    type Error = Error;

    fn try_from(value: &'a ZendHashTable) -> Result<Self> {
        if value.len() != N {
            return Err(Error::IntegerOverflow);
        }

        let mut temp_array: [MaybeUninit<T>; N] = unsafe { MaybeUninit::uninit().assume_init() };

        let mut iter = value.into_iter();

        for (i, elem) in temp_array.iter_mut().enumerate() {
            let (_, val) = iter.next().ok_or(Error::IntegerOverflow)?;

            let Some(converted) = T::from_zval(val) else {
                for initialized in temp_array.iter_mut().take(i) {
                    unsafe {
                        initialized.assume_init_drop();
                    }
                }
                return Err(Error::ZvalConversion(val.get_type()));
            };

            *elem = MaybeUninit::new(converted);
        }

        let final_array: [T; N] = unsafe { std::mem::transmute_copy(&temp_array) };

        Ok(final_array)
    }
}

impl<T, const N: usize> TryFrom<[T; N]> for ZBox<ZendHashTable>
where
    T: IntoZval,
{
    type Error = Error;

    fn try_from(value: [T; N]) -> Result<Self> {
        let mut ht =
            ZendHashTable::with_capacity(N.try_into().map_err(|_| Error::IntegerOverflow)?);

        for val in value {
            ht.push(val)?;
        }

        Ok(ht)
    }
}

impl<T, const N: usize> IntoZval for [T; N]
where
    T: IntoZval,
{
    const TYPE: DataType = DataType::Array;
    const NULLABLE: bool = false;

    fn set_zval(self, zv: &mut Zval, _: bool) -> Result<()> {
        let arr = self.try_into()?;
        zv.set_hashtable(arr);
        Ok(())
    }
}

impl<'a, T, const N: usize> FromZval<'a> for [T; N]
where
    T: FromZval<'a>,
{
    const TYPE: DataType = DataType::Array;

    fn from_zval(zval: &'a Zval) -> Option<Self> {
        if !zval.is_array() {
            return None; // Évite le plantage si le type est Long ou String
        }
        zval.array().and_then(|arr| <[T; N]>::try_from(arr).ok())
    }
}

#[cfg(test)]
#[cfg(feature = "embed")]
#[allow(clippy::unwrap_used)]
mod tests {
    use crate::boxed::ZBox;
    use crate::convert::{FromZval, IntoZval};
    use crate::embed::Embed;
    use crate::error::Error;
    use crate::types::{ZendHashTable, Zval};

    #[test]
    fn test_hash_table_try_from_array() {
        Embed::run(|| {
            let arr = ["value1", "value2", "value3"];

            let ht: ZBox<ZendHashTable> = arr.try_into().unwrap();
            assert_eq!(ht.len(), 3);
            assert_eq!(ht.get(0).unwrap().string().unwrap(), "value1");
            assert_eq!(ht.get(1).unwrap().string().unwrap(), "value2");
            assert_eq!(ht.get(2).unwrap().string().unwrap(), "value3");

            let converted_arr: [String; 3] = ht.as_ref().try_into().unwrap();
            assert_eq!(converted_arr, ["value1".to_string(), "value2".to_string(), "value3".to_string()]);
        });
    }

    #[test]
    fn test_array_into_zval() {
        Embed::run(|| {
            let arr = ["value1", "value2"];

            let zval = arr.into_zval(false).unwrap();
            assert!(zval.is_array());
            let ht: &ZendHashTable = zval.array().unwrap();
            assert_eq!(ht.len(), 2);
            assert_eq!(ht.get(0).unwrap().string().unwrap(), "value1");
            assert_eq!(ht.get(1).unwrap().string().unwrap(), "value2");
        });
    }

    #[test]
    fn test_array_from_zval() {
        Embed::run(|| {
            let mut ht = ZendHashTable::new();
            ht.push("value1").unwrap();
            ht.push("value2").unwrap();
            ht.push("value3").unwrap();
            let mut zval = Zval::new();
            zval.set_hashtable(ht);

            let arr: [String; 3] = <[String; 3]>::from_zval(&zval).unwrap();
            assert_eq!(arr, ["value1".to_string(), "value2".to_string(), "value3".to_string()]);

            // Test wrong size returns None
            let wrong_arr: Option<[String; 2]> = <[String; 2]>::from_zval(&zval);
            assert!(wrong_arr.is_none());
        });
    }
}