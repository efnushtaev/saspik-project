import { createPortal } from 'react-dom';
import { createCn } from 'bem-react-classname';

import './styles.css';

const cn = createCn('confirm-modal');

interface ConfirmModalProps {
  open: boolean;
  title: string;
  message?: string;
  confirmLabel?: string;
  cancelLabel?: string;
  danger?: boolean;
  submitting?: boolean;
  onConfirm: () => void;
  onClose: () => void;
}

export const ConfirmModal = ({
  open,
  title,
  message,
  confirmLabel = 'Подтвердить',
  cancelLabel = 'Отмена',
  danger = false,
  submitting = false,
  onConfirm,
  onClose,
}: ConfirmModalProps) => {
  if (!open) {
    return null;
  }

  return createPortal(
    <div className={cn()} onClick={onClose}>
      <div className={cn('dialog')} onClick={e => e.stopPropagation()}>
        <div className={cn('title')}>{title}</div>
        {message && <div className={cn('message')}>{message}</div>}
        <div className={cn('actions')}>
          <button type="button" className={cn('button')} onClick={onClose}>
            {cancelLabel}
          </button>
          <button
            type="button"
            className={cn('button', { primary: !danger, danger })}
            onClick={onConfirm}
            disabled={submitting}
          >
            {submitting ? 'Подождите…' : confirmLabel}
          </button>
        </div>
      </div>
    </div>,
    document.body,
  );
};
