pub type Nat = usize;
pub type Span = std::ops::Range<Nat>;

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
