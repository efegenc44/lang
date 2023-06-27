pub type Nat = usize;
pub type Int = isize;
pub type Span = std::ops::Range<Nat>;
pub type Symbol = Box<str>;

#[cfg(target_pointer_width = "64")]
pub type Real = f64;
#[cfg(target_pointer_width = "32")]
pub type Real = f32;

#[derive(Debug)]
pub struct Spanned<Data> {
    pub data: Data,
    pub span: Span,
}

pub trait HasSpan {
    fn spanned(self, span: Span) -> Spanned<Self>
    where
        Self: Sized,
    {
        Spanned { data: self, span }
    }

    fn start_end(self, start_span: Span, end_span: Span) -> Spanned<Self>
    where
        Self: Sized,
    {
        debug_assert!(start_span.start < end_span.end);
        self.spanned(start_span.start..end_span.end)
    }
}

pub trait Error {
    fn message(&self) -> String;
}

impl<T: Error> std::fmt::Display for Spanned<T> {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        const ERROR_INDENT_LENGTH: usize = 4;

        writeln!(f)?;
        writeln!(
            f,
            "{:indent$} | {}:{}",
            "",
            self.span.start,
            self.span.end,
            indent = ERROR_INDENT_LENGTH
        )?;
        writeln!(
            f,
            "{:indent$} | {}",
            "",
            self.data.message(),
            indent = ERROR_INDENT_LENGTH
        )
    }
}

#[macro_export]
macro_rules! repl_handle_error {
    ($e:expr) => {
        match $e {
            Ok(data) => data,
            Err(error) => {
                println!("{error}");
                continue;
            }
        }
    };
}
